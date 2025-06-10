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
BL31 := arm-trusted-firmware/build/rpi4/debug/bl31.bin

.PHONY: clean rootfs u-boot linux

prepare:
	git submodule init
	git submodule update --progress --depth=1

$(BL31): tfa

image: $(UBOOT) $(KERNEL) $(DTB_IMAGE) $(BOOT_SCR) $(BL31)

u-boot:
	cp -rfv configs/rpi_4_user_defconfig u-boot/configs/
	cd u-boot && export CROSS_COMPILE=${CROSS_PREFIX} && \
		make clean && make rpi_4_user_defconfig && make -j16
	ls -al u-boot/u-boot.bin
	@echo "[INFO] uboot build!"

bootscr:
	./u-boot/tools/mkimage -C none -A arm64 -T script -d configs/boot.cmd boot.scr

tfa:
	cd arm-trusted-firmware && export CROSS_COMPILE=${CROSS_PREFIX} && \
		make PLAT=rpi4 DEBUG=1 LOG_LEVEL=50 -j16

linux:
	cd linux && export KERNEL=kernel8 && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} bcm2711_defconfig && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} -j16

ramdisk:
	aria2c -x5 "${RAMDISK_TAR}" || rm -rf ${RAMDISK} && ls ${RAMDISK}
	ln -s rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz ramdisk.cpio.gz

rootfs:
	ls $(ROOTFSDISK) || aria2c -x5 "${ROOTFS_TAR}" || rm -rf ${ROOTFSDISK} && ls ${ROOTFSDISK}

$(KERNEL_IMAGE): linux

$(DTB_IMAGE): linux

$(RAMDISK): ramdisk

fit_noram: $(KERNEL_IMAGE) $(DTB_IMAGE)
	@echo "[INFO] Gen key."
	openssl genpkey -algorithm RSA -out keys/dev.key -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:65537
	openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt
	openssl rsa -in keys/dev.key -pubout
	@echo "[INFO] Backup dtb. because mkimage will change dtb file."
	# ==> Please note that, Rapsi 4B board will read the linux device tree from the boot partition of the SD card instead of uboot's FDT.
	# ==> Therefore, the uboot FDT will be bypassed. I have provided a common method for creating uboot fdt as follows:
	# ==>  cp -rfv u-boot/arch/arm/dts/bcm2711-rpi-4-b.dtb u-boot/arch/arm/dts/bcm2711-rpi-4-b-signature.dtb
	# ==> ./u-boot/tools/mkimage -f configs/raspi_noramdisk.its -k keys -K u-boot/arch/arm/dts/bcm2711-rpi-4-b-signature.dtb -r image.ub
	# ==> Ops!!! In order to adapt the Rapsi 4B board, I have to create a dtb with public key using Linux's device tree.
	# ==> Then, copy the ..../bcm2711-rpi-4-b-signature.dtb to the boot partition of the SD card and rename it to bcm2711-rpi-4-b.dtb
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb
	./u-boot/tools/mkimage -f configs/raspi_noramdisk.its -k keys -K linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb -r image.ub
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb bcm2711-rpi-4-b.dtb
	sha256sum image.ub
	./u-boot/tools/fit_check_sign -f image.ub -k bcm2711-rpi-4-b.dtb
	@echo "[INFO] Rebuild U-boot to add fdt to uboot.bin."
	# ==> Ops!!! It is not nessasery to Rapsi 4B board, but to a normal hardware platform.
	cd u-boot && export CROSS_COMPILE=${CROSS_PREFIX} && EXT_DTB=../linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb make -j16
	@echo "[INFO] !!!!! Please remember to copy bcm2711-rpi-4-b.dtb to your boot partition of your sd card!!! The public key is in bcm2711-rpi-4-b.dtb"

fit: $(KERNEL_IMAGE) $(DTB_IMAGE)
	openssl genpkey -algorithm RSA -out keys/dev.key -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:65537
	openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt
	openssl rsa -in keys/dev.key -pubout
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb
	./u-boot/tools/mkimage -f configs/raspi.its -k keys -K linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb -r image.ub
	cp -rfv linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb bcm2711-rpi-4-b.dtb
	sha256sum image.ub
	./u-boot/tools/fit_check_sign -f image.ub -k bcm2711-rpi-4-b.dtb
	@echo "[INFO] Rebuild U-boot to add fdt to uboot.bin."
	# ==> Ops!!! It is not nessasery to Rapsi 4B board, but to a normal hardware platform.
	cd u-boot && export CROSS_COMPILE=${CROSS_PREFIX} && EXT_DTB=../linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb make -j16
	@echo "[INFO] !!!!! Please remember to copy bcm2711-rpi-4-b.dtb to your boot partition of your sd card!!! The public key is in bcm2711-rpi-4-b.dtb"

$(BOOT_SCR): bootscr

$(KERNEL): fit_noram

$(UBOOT): u-boot

$(ROOTFSDISK): rootfs

sd: $(ROOTFSDISK) $(BOOT_SCR) $(KERNEL) $(UBOOT) $(BL31)
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
	sudo cp -rfv bcm2711-rpi-4-b.dtb boot
	sudo cp -rfv firmware/boot/start4.elf boot
	sudo cp -rfv firmware/boot/bootcode.bin boot
	sudo cp -rfv configs/config.txt boot
	sudo cp -rfv $(BL31) boot
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
	rm -rf boot.scr ramdisk.cpio.gz root.tar.xz root.tar.xz bcm2711-rpi-4-b.dtb
	rm -rf image.ub rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz
	rm -rf keys/dev.*

distclean:
	rm -rf rootfs boot linux u-boot firmware rpi-buildroot
	rm -rf 2023-12-11-raspios-bookworm-arm64-lite.img*
	make clean
