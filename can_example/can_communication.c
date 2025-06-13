#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

/* CAN Parameter Configuration
 * 1. Device: Uses can0 interface (ensure hardware support, e.g., MCP2515 module).
 * 2. Bitrate: Set to 100000 bps (100kbps), adjust based on ECU requirements.
 * 3. CAN ID: Sending uses 0x123 (or 0x7E0 for UDS), receiving accepts all IDs or filters for 0x123.
 * 4. Data: Sends string "hello world hello can" or UDS request (e.g., 0x02 0x10 0x03).
 *
 * Linux Health Check Steps:
 * - Verify CAN module: `lsmod | grep mcp251x`
 * - Check CAN interface: `ip link show can0`
 * - Configure bitrate and enable interface:
 *   sudo ip link set can0 type can bitrate 100000
 *   sudo ip link set can0 up
 * - Monitor CAN traffic: `candump can0`
 * - Check error frames: `cat /proc/net/can/stats`
 * - Ensure proper termination resistor (120Ω) to avoid signal reflection.
 */

/* Configuration Options */
#define USE_CAN_FILTER 1 // Set to 1 to filter for can_id=0x123, 0 for all frames
#define USE_BLOCKING_READ 0 // Set to 1 for blocking read, 0 for select-based read

/* Global variables for thread control */
static volatile int32_t running = 1;
static int32_t socket_fd = -1;

/* Initialize CAN socket */
int32_t init_can_socket(const char *ifname)
{
    struct ifreq ifr;
    struct sockaddr_can addr;
    int32_t ret;

    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd < 0) {
        perror("Failed to create CAN socket");
        return -1;
    }

    // Increase receive buffer size
    int32_t rcvbuf_size = 1048576; // 1MB
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof(rcvbuf_size));
    if (ret < 0) {
        perror("Failed to set receive buffer size");
        close(socket_fd);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ret = ioctl(socket_fd, SIOCGIFINDEX, &ifr);
    if (ret < 0) {
        perror("Failed to get CAN interface index");
        close(socket_fd);
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    ret = bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Failed to bind CAN socket");
        close(socket_fd);
        return -1;
    }

    // Configure CAN filter
    if (USE_CAN_FILTER) {
        struct can_filter rfilter[1];
        rfilter[0].can_id = 0x123;
        rfilter[0].can_mask = CAN_SFF_MASK; // Standard 11-bit ID mask
        ret = setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
        if (ret < 0) {
            perror("Failed to set CAN filter");
            close(socket_fd);
            return -1;
        }
    } else {
        ret = setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
        if (ret < 0) {
            perror("Failed to disable CAN filters");
            close(socket_fd);
            return -1;
        }
    }

    // Disable loopback to avoid receiving own messages
    int32_t loopback = 0;
    ret = setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
    if (ret < 0) {
        perror("Failed to disable CAN loopback");
        close(socket_fd);
        return -1;
    }

    return 0;
}

/* Send CAN frame with specified data */
int32_t send_can_frame(int32_t sock, uint32_t can_id, const uint8_t *data, size_t len)
{
    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));

    frame.can_id = can_id;
    frame.can_dlc = (len > 8) ? 8 : len;
    memcpy(frame.data, data, frame.can_dlc);

    printf("Sending CAN frame: ID=0x%X, DLC=%d, Data=", frame.can_id, frame.can_dlc);
    for (size_t i = 0; i < frame.can_dlc; i++) {
        printf("0x%02X ", frame.data[i]);
    }
    printf("\n");

    int32_t nbytes = write(sock, &frame, sizeof(frame));
    if (nbytes != sizeof(frame)) {
        perror("Failed to send CAN frame");
        return -1;
    }

    return 0;
}

/* Receive and print CAN frame */
int32_t receive_can_frame(int32_t sock)
{
    struct can_frame frame;
    int32_t nbytes = read(sock, &frame, sizeof(frame));
    if (nbytes < 0) {
        perror("Failed to receive CAN frame");
        return -1;
    }
    if (nbytes == sizeof(frame)) {
        printf("Received CAN frame: ID=0x%X, DLC=%d, Data=", frame.can_id, frame.can_dlc);
        for (size_t i = 0; i < frame.can_dlc; i++) {
            printf("0x%02X ", frame.data[i]);
        }
        printf("\n");
        return 0;
    }
    printf("Incomplete CAN frame received: %d bytes\n", nbytes);
    return -1;
}

/* Sending thread function */
void *send_thread(void *arg)
{
    const char *message = "hello world hello can";
    size_t msg_len = strlen(message);
    uint32_t can_id = 0x123;
    // For UDS: uint32_t can_id = 0x7E0; const uint8_t uds_request[] = {0x02, 0x10, 0x03};

    while (running) {
        size_t offset = 0;
        while (offset < msg_len && running) {
            size_t chunk_len = (msg_len - offset > 8) ? 8 : (msg_len - offset);
            if (send_can_frame(socket_fd, can_id, (uint8_t *)message + offset, chunk_len) < 0) {
                printf("Send error in thread, continuing\n");
            }
            offset += chunk_len;
        }
        sleep(1); // Send every 1 second
    }
    return NULL;
}

/* Receiving thread function */
void *receive_thread(void *arg)
{
    if (USE_BLOCKING_READ) {
        // Blocking read, similar to supplier's approach
        while (running) {
            if (receive_can_frame(socket_fd) == 0) {
                printf("Successfully received a frame\n");
            }
            usleep(10000); // Small delay to avoid CPU overload
        }
    } else {
        // Non-blocking read with select
        while (running) {
            struct timeval tv;
            fd_set read_fds;
            tv.tv_sec = 1; // 1s timeout
            tv.tv_usec = 0;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd, &read_fds);

            int32_t ret = select(socket_fd + 1, &read_fds, NULL, NULL, &tv);
            if (ret < 0) {
                perror("Select error in receive thread");
            } else if (ret > 0 && FD_ISSET(socket_fd, &read_fds)) {
                if (receive_can_frame(socket_fd) == 0) {
                    printf("Successfully received a frame\n");
                }
            }
        }
    }
    return NULL;
}

/* Signal handler for clean shutdown */
void signal_handler(int32_t sig)
{
    running = 0;
}

int main()
{
    const char *ifname = "can0";
    pthread_t send_tid, receive_tid;

    /* Setup signal handler for Ctrl+C */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Setup CAN interface */
    printf("Setting up CAN interface: %s\n", ifname);
    system("sudo ip link set can0 type can bitrate 100000");
    system("sudo ip link set can0 up");

    /* Initialize CAN socket */
    if (init_can_socket(ifname) < 0) {
        system("sudo ip link set can0 down");
        return 1;
    }

    /* Create sending and receiving threads */
    if (pthread_create(&send_tid, NULL, send_thread, NULL) != 0) {
        perror("Failed to create send thread");
        close(socket_fd);
        system("sudo ip link set can0 down");
        return 1;
    }
    if (pthread_create(&receive_tid, NULL, receive_thread, NULL) != 0) {
        perror("Failed to create receive thread");
        running = 0;
        pthread_join(send_tid, NULL);
        close(socket_fd);
        system("sudo ip link set can0 down");
        return 1;
    }

    /* Wait for threads to finish (on signal) */
    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    /* Cleanup */
    close(socket_fd);
    system("sudo ip link set can0 down");
    printf("CAN communication stopped\n");
    return 0;
}