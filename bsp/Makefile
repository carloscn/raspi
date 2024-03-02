include build.cfg
export $(shell sed 's/=.*//' build.cfg)

SHELL := /bin/bash
BOOT_SCR := boot.scr
KERNEL := image.ub
UBOOT := u-boot/u-boot.bin
KERNEL_IMAGE := linux/arch/arm64/boot/Image.gz
DTB_IMAGE := linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb
RAMDISK := rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz
ROOTFSDISK := 2023-12-11-raspios-bookworm-arm64-lite.img.xz

.PHONY: clean rootfs

prepare:
	git submodule init
	git submodule update --progress --depth=1

image: $(UBOOT) $(KERNEL) $(DTB_IMAGE) $(BOOT_SCR)

u-boot:
	cp -rfv configs/rpi_4_user_defconfig u-boot/configs/
	cd u-boot && export CROSS_COMPILE=${CROSS_PREFIX} && \
		make clean && make rpi_4_user_defconfig && make -j16
	ls -al u-boot/u-boot.bin
	@echo "[INFO] uboot build!"

bootscr:
	mkimage -C none -A arm64 -T script -d configs/boot.cmd boot.scr

linux:
	cd linux && export KERNEL=kernel8 && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} bcm2711_defconfig && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} -j16

ramdisk:
	aria2c -x5 "${RAMDISK_TAR}" || rm -rf ${RAMDISK_TAR} && ls ${RAMDISK_TAR}
	ln -s rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz ramdisk.cpio.gz

rootfs:
	ls $(ROOTFSDISK) || aria2c -x5 "${ROOTFS_TAR}" || rm -rf ${ROOTFSDISK} && ls ${ROOTFSDISK}

$(KERNEL_IMAGE): linux

$(DTB_IMAGE): linux

$(RAMDISK): ramdisk

fit: $(KERNEL_IMAGE) $(DTB_IMAGE) $(RAMDISK)
	openssl genpkey -algorithm RSA -out keys/dev.key -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:65537
	openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt
	openssl rsa -in keys/dev.key -pubout
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb
	mkimage -f configs/raspi.its -K linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb -k keys image.ub
	sha256sum image.ub

fit_noram: $(KERNEL_IMAGE) $(DTB_IMAGE)
	@echo "[INFO] Gen key."
	openssl genpkey -algorithm RSA -out keys/dev.key -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:65537
	openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt
	openssl rsa -in keys/dev.key -pubout
	@echo "[INFO] Backup dtb. because mkimage will change dtb file."
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb
	mkimage -f configs/raspi_noramdisk.its -K linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb -k keys image.ub
	sha256sum image.ub

$(BOOT_SCR): bootscr

$(KERNEL): fit_noram

$(UBOOT): u-boot

$(ROOTFSDISK): rootfs

sd: $(ROOTFSDISK) $(BOOT_SCR) $(KERNEL) $(UBOOT)
	-sudo umount ${SD_DEV}1
	-sudo umount ${SD_DEV}2
	rm -rf boot rootfs && mkdir -p boot rootfs
	xz -k -f -vv -d "2023-12-11-raspios-bookworm-arm64-lite.img.xz"
	sudo dd if=2023-12-11-raspios-bookworm-arm64-lite.img of=${SD_DEV} bs=32M status=progress
	sync
	@echo "[INFO] Remounting ${SD_DEV} to the host."
	sudo mount ${SD_DEV}1 boot
	sudo mount ${SD_DEV}2 rootfs
	sudo rm -rf boot/* && sync
	@echo "[INFO] Copying boot binaries to sd card."
	#sudo cp -rfv firmware/boot/bcm2711-rpi-4-b.dtb boot
	sudo cp -rfv firmware/boot/start4.elf boot
	sudo cp -rfv firmware/boot/bootcode.bin boot
	sudo cp -rfv configs/config.txt boot
	@echo "[INFO] Create username:${USER_NAME} and password:123456 for rootfs"
	echo "${USER_NAME}:$(openssl passwd -6 -stdin <<<'123456')" > userconf.txt
	sudo cp -rfv userconf.txt boot && rm -rf userconf.txt
	@echo "[INFO] Copying u-boot as kernel8.img to sd card."
	sudo cp -rfv u-boot/u-boot.bin boot/kernel8.img
	sudo cp -rfv boot.scr boot/
	@echo "[INFO] Copying Linux kernel to sd card."
	sudo cp -rfv image.ub boot/
	@echo "[INFO] Syncing disk."
	sync
	@echo "[INFO] Copying to sd card has been done."
	sudo umount ${SD_DEV}1
	sudo umount ${SD_DEV}2
	rm -rf boot rootfs

clean:
	rm -rf boot.scr ramdisk.cpio.gz root.tar.xz root.tar.xz
	rm -rf image.ub rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz
	rm -rf keys/dev.*

distclean:
	rm -rf rootfs boot linux u-boot firmware rpi-buildroot
	rm -rf 2023-12-11-raspios-bookworm-arm64-lite.img*
	make clean
