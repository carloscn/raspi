part uuid mmc 0:2 uuid
setenv bootargs 8250.nr_uarts=1 console=ttyS0,115200 console=tty1 root=PARTUUID=${uuid} rootfstype=ext4 fsck.repair=yes rootwait
fatload mmc 0:1 4000000 image.ub
bootm 4000000#raspberrypi-os-64