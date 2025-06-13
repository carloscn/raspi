#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/isotp.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

/* CAN-TP Parameter Configuration
 * 1. Device: Uses can0 interface (ensure hardware support, e.g., MCP2515 module).
 * 2. Bitrate: Set to 100000 bps (100kbps), adjust based on ECU requirements.
 * 3. CAN ID: Sending uses 0x7E0, receiving uses 0x7E8.
 * 4. Data: Sends 100-byte CAN-TP messages to test multi-frame transmission.
 *
 * Linux Health Check Steps:
 * - Verify CAN module: `lsmod | grep mcp251x`
 * - Install CANTP module: `sudo modprobe can-isotp`
 * - Verify CANTP moudle: `lsmod | grep can_isotp`
 * - Check CAN interface: `ip link show can0`
 * - Configure bitrate and enable interface:
 *   `sudo ip link set can0 type can bitrate 100000`
 *   `sudo ip link set can0 up`
 * - Monitor CAN traffic: `candump can0`
 * - Check error frames: `cat /proc/net/can/stats`
 * - Ensure proper termination resistor (120Ω) to avoid signal reflection.
 */

/* Global variables for thread control */
static volatile int32_t running = 1;
static int32_t socket_fd = -1;

/* Initialize ISO-TP socket */
int32_t init_isotp_socket(const char *ifname, uint32_t tx_id, uint32_t rx_id) {
    struct ifreq ifr;
    struct sockaddr_can addr;
    int32_t ret;

    socket_fd = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP);
    if (socket_fd < 0) {
        perror("Failed to create ISO-TP socket");
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

    // Configure flow control options
    struct can_isotp_fc_options fc_opts = {
        .bs = 8,    // Block size
        .stmin = 5, // Separation time (5ms)
        .wftmax = 0 // Max wait frames
    };
    ret = setsockopt(socket_fd, SOL_CAN_ISOTP, CAN_ISOTP_RECV_FC, &fc_opts, sizeof(fc_opts));
    if (ret < 0) {
        perror("Failed to set flow control options");
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
    addr.can_addr.tp.tx_id = tx_id;
    addr.can_addr.tp.rx_id = rx_id;
    ret = bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Failed to bind ISO-TP socket");
        close(socket_fd);
        return -1;
    }

    return 0;
}

/* Sending thread function */
void *send_thread(void *arg) {
    // 100-byte CAN-TP payload for multi-frame testing
    uint8_t cantp_payload[100];
    for (size_t i = 0; i < sizeof(cantp_payload); i++) {
        cantp_payload[i] = (uint8_t)i; // Fill with 0x00, 0x01, ..., 0x63
    }
    size_t msg_len = sizeof(cantp_payload);

    while (running) {
        int32_t nbytes = send(socket_fd, cantp_payload, msg_len, 0);
        if (nbytes < 0) {
            perror("Failed to send CAN-TP message");
        } else {
            printf("Sent CAN-TP message (%zu bytes): ", msg_len);
            for (size_t i = 0; i < msg_len && i < 16; i++) { // Print first 16 bytes
                printf("0x%02X ", cantp_payload[i]);
            }
            printf("...\n");
        }
        sleep(1); // Send every 1 second
    }
    return NULL;
}

/* Receiving thread function */
void *receive_thread(void *arg) {
    uint8_t buffer[4095]; // Max ISO-TP payload
    while (running) {
        int32_t nbytes = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (nbytes > 0) {
            printf("Received CAN-TP message (%d bytes): ", nbytes);
            for (int32_t i = 0; i < nbytes && i < 16; i++) { // Print first 16 bytes
                printf("0x%02X ", buffer[i]);
            }
            printf("...\n");
        }
    }
    return NULL;
}

/* Signal handler for clean shutdown */
void signal_handler(int32_t sig) {
    running = 0;
}

int main() {
    const char *ifname = "can0";
    uint32_t tx_id = 0x7E0; // CAN-TP request ID
    uint32_t rx_id = 0x7E8; // CAN-TP response ID
    pthread_t send_tid, receive_tid;

    /* Setup signal handler for Ctrl+C */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Setup CAN interface */
    printf("Setting up CAN interface: %s\n", ifname);
    system("sudo ip link set can0 type can bitrate 100000");
    system("sudo ip link set can0 up");

    /* Initialize ISO-TP socket */
    if (init_isotp_socket(ifname, tx_id, rx_id) < 0) {
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
    printf("CAN-TP communication stopped\n");
    return 0;
}