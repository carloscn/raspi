# Include configuration from build.cfg
include build.cfg
export $(shell sed 's/=.*//' build.cfg)

# Use bash as the shell for consistent behavior
SHELL := /bin/bash

# File and directory definitions
BOOT_SCR := boot.scr
KERNEL := image.ub
UBOOT := u-boot/u-boot.bin
KERNEL_IMAGE := linux/arch/arm64/boot/Image.gz
DTB_IMAGE := linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb
RAMDISK := rootfs_lsdk2012_yocto_tiny_arm64.cpio.gz
ROOTFSDISK := 2023-12-11-raspios-bookworm-arm64-lite.img
BL31 := arm-trusted-firmware/build/rpi4/debug/bl31.bin
FIRMWARE_DIR := firmware/boot

# ANSI color codes for output
RED=\033[0;31m
GREEN=\033[0;32m
BLUE=\033[0;34m
NC=\033[0m

# Phony targets (not associated with files)
.PHONY: all prepare u-boot bootscr tfa linux ramdisk rootfs fit clean distclean sd image

# Default target
all: image sd

# Initialize and update submodules
prepare:
	@echo -e "$(BLUE)[INFO]$(NC) Initializing and updating submodules."
	@git submodule init
	@git submodule update --progress --depth=1 --jobs=4 || { echo -e "$(RED)[ERROR]$(NC) Failed to update submodules."; exit 1; }

# Build U-Boot
u-boot:
	@echo -e "$(BLUE)[INFO]$(NC) Building U-Boot."
	@cp configs/rpi_4_user_defconfig u-boot/configs/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy U-Boot config."; exit 1; }
	@cd u-boot && export CROSS_COMPILE=${CROSS_PREFIX} && \
		make clean && make rpi_4_user_defconfig && make -j$(shell nproc) || { echo -e "$(RED)[ERROR]$(NC) Failed to build U-Boot."; exit 1; }
	@echo -e "$(GREEN)[SUCCESS]$(NC) U-Boot built: $(UBOOT)."

# Generate boot script
bootscr:
	@echo -e "$(BLUE)[INFO]$(NC) Generating boot script."
	@./u-boot/tools/mkimage -C none -A arm64 -T script -d configs/boot.cmd $(BOOT_SCR) || { echo -e "$(RED)[ERROR]$(NC) Failed to generate boot.scr."; exit 1; }
	@echo -e "$(GREEN)[SUCCESS]$(NC) Boot script generated: $(BOOT_SCR)."

# Build Trusted Firmware-A (TFA)
tfa:
	@echo -e "$(BLUE)[INFO]$(NC) Building Trusted Firmware-A."
	@cd arm-trusted-firmware && export CROSS_COMPILE=${CROSS_PREFIX} && \
		make PLAT=rpi4 DEBUG=1 LOG_LEVEL=50 -j$(shell nproc) || { echo -e "$(RED)[ERROR]$(NC) Failed to build TFA."; exit 1; }
	@echo -e "$(GREEN)[SUCCESS]$(NC) TFA built: $(BL31)."

# Build Linux kernel
linux:
	@echo -e "$(BLUE)[INFO]$(NC) Building Linux kernel."
	@cd linux && export KERNEL=kernel8 && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} bcm2711_defconfig && \
		make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} -j$(shell nproc) || { echo -e "$(RED)[ERROR]$(NC) Failed to build Linux kernel."; exit 1; }
	@echo -e "$(GREEN)[SUCCESS]$(NC) Linux kernel built: $(KERNEL_IMAGE), $(DTB_IMAGE)."

# Download ramdisk
ramdisk:
	@echo -e "$(BLUE)[INFO]$(NC) Downloading ramdisk."
	@if [ ! -f $(RAMDISK) ]; then \
		aria2c -x5 "$(RAMDISK_TAR)" || { echo -e "$(RED)[ERROR]$(NC) Failed to download ramdisk."; rm -f $(RAMDISK); exit 1; }; \
		ln -sf $(RAMDISK) ramdisk.cpio.gz || { echo -e "$(RED)[ERROR]$(NC) Failed to create ramdisk symlink."; exit 1; }; \
	else \
		echo -e "$(BLUE)[INFO]$(NC) Ramdisk already exists: $(RAMDISK)."; \
	fi
	@echo -e "$(GREEN)[SUCCESS]$(NC) Ramdisk prepared: $(RAMDISK)."

# Download root filesystem
rootfs:
	@echo -e "$(BLUE)[INFO]$(NC) Downloading root filesystem."
	@if [ ! -f $(ROOTFSDISK).xz ]; then \
		aria2c -x5 "$(ROOTFS_TAR)" || { echo -e "$(RED)[ERROR]$(NC) Failed to download rootfs."; rm -f $(ROOTFSDISK).xz; exit 1; }; \
	else \
		echo -e "$(BLUE)[INFO]$(NC) Rootfs already exists: $(ROOTFSDISK).xz."; \
	fi
	@echo -e "$(GREEN)[SUCCESS]$(NC) Rootfs prepared: $(ROOTFSDISK).xz."

# Generate FIT image (consolidated fit and fit_noram)
fit: $(KERNEL_IMAGE) $(DTB_IMAGE)
	@echo -e "$(BLUE)[INFO]$(NC) Generating FIT image."
	@mkdir -p keys
	@openssl genpkey -algorithm RSA -out keys/dev.key -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:65537 || { echo -e "$(RED)[ERROR]$(NC) Failed to generate RSA key."; exit 1; }
	@openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt || { echo -e "$(RED)[ERROR]$(NC) Failed to generate certificate."; exit 1; }
	@openssl rsa -in keys/dev.key -pubout || { echo -e "$(RED)[ERROR]$(NC) Failed to extract public key."; exit 1; }
	@cp linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb || { echo -e "$(RED)[ERROR]$(NC) Failed to backup DTB."; exit 1; }
	@./u-boot/tools/mkimage -f configs/raspi_noramdisk.its -k keys -K linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb -r $(KERNEL) || { echo -e "$(RED)[ERROR]$(NC) Failed to generate FIT image."; exit 1; }
	@cp linux/arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b-signature.dtb bcm2711-rpi-4-b.dtb || { echo -e "$(RED)[ERROR]$(NC) Failed to copy signed DTB."; exit 1; }
	@sha256sum $(KERNEL) || { echo -e "$(RED)[ERROR]$(NC) Failed to compute SHA256 for $(KERNEL)."; exit 1; }
	@./u-boot/tools/fit_check_sign -f $(KERNEL) -k bcm2711-rpi-4-b.dtb || { echo -e "$(RED)[ERROR]$(NC) FIT signature check failed."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Note: Copy bcm2711-rpi-4-b.dtb to the boot partition of the SD card (public key included)."
	@echo -e "$(GREEN)[SUCCESS]$(NC) FIT image generated: $(KERNEL)."

# Build all image components
image: $(UBOOT) $(BOOT_SCR) $(KERNEL) $(DTB_IMAGE) $(BL31)

# Prepare SD card
sd: $(ROOTFSDISK) $(BOOT_SCR) $(KERNEL) $(UBOOT) $(BL31)
	@echo -e "$(BLUE)[INFO]$(NC) Unmounting $(SD_DEV)1 and $(SD_DEV)2 if mounted."
	@sudo -E umount $(SD_DEV)1 2>/dev/null || true
	@sudo -E umount $(SD_DEV)2 2>/dev/null || true
	@echo -e "$(BLUE)[INFO]$(NC) Setting up temporary directories."
	@rm -rf boot rootfs
	@mkdir -p boot rootfs || { echo -e "$(RED)[ERROR]$(NC) Failed to create boot/rootfs directories."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Decompressing $(ROOTFSDISK).xz."
	@xz -k -f -d "$(ROOTFSDISK).xz" || { echo -e "$(RED)[ERROR]$(NC) Failed to decompress $(ROOTFSDISK).xz."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Writing image to $(SD_DEV)."
	@sudo -E dd if=$(ROOTFSDISK) of=$(SD_DEV) bs=32M status=progress || { echo -e "$(RED)[ERROR]$(NC) Failed to write image to $(SD_DEV)."; exit 1; }
	@sync || { echo -e "$(RED)[ERROR]$(NC) Sync failed after dd."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Mounting $(SD_DEV)1 and $(SD_DEV)2."
	@sudo -E mount $(SD_DEV)1 boot || { echo -e "$(RED)[ERROR]$(NC) Failed to mount $(SD_DEV)1."; exit 1; }
	@sudo -E mount $(SD_DEV)2 rootfs || { echo -e "$(RED)[ERROR]$(NC) Failed to mount $(SD_DEV)2."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Clearing boot partition contents."
	@sudo -E rm -rf boot/* || { echo -e "$(RED)[ERROR]$(NC) Failed to clear boot partition."; exit 1; }
	@sync
	@echo -e "$(BLUE)[INFO]$(NC) Copying boot binaries to SD card."
	@sudo -E cp -f bcm2711-rpi-4-b.dtb boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy bcm2711-rpi-4-b.dtb."; exit 1; }
	@sudo -E cp -f $(FIRMWARE_DIR)/start4.elf boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy start4.elf."; exit 1; }
	@sudo -E cp -f $(FIRMWARE_DIR)/bootcode.bin boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy bootcode.bin."; exit 1; }
	@sudo -E cp -f configs/config.txt boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy config.txt."; exit 1; }
	@sudo -E cp -f $(BL31) boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy $(BL31)."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Creating user $(USER_NAME) with password."
	@echo "$(USER_NAME):$$(openssl passwd -6 -stdin <<< '$(PASSWD)')" > userconf.txt || { echo -e "$(RED)[ERROR]$(NC) Failed to create userconf.txt."; exit 1; }
	@sudo -E cp -f userconf.txt boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy userconf.txt."; exit 1; }
	@rm -f userconf.txt
	@echo -e "$(BLUE)[INFO]$(NC) Copying U-Boot as kernel8.img."
	@sudo -E cp -f $(UBOOT) boot/kernel8.img || { echo -e "$(RED)[ERROR]$(NC) Failed to copy U-Boot to kernel8.img."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Copying boot script and kernel."
	@sudo -E cp -f $(BOOT_SCR) boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy $(BOOT_SCR)."; exit 1; }
	@sudo -E cp -f $(KERNEL) boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy $(KERNEL)."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Copying ssh boot script."
	@sudo -E cp -f configs/ssh boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy ssh."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Syncing and unmounting."
	@sync || { echo -e "$(RED)[ERROR]$(NC) Sync failed."; exit 1; }
	@sudo -E umount $(SD_DEV)1 || { echo -e "$(RED)[ERROR]$(NC) Failed to unmount $(SD_DEV)1."; exit 1; }
	@sudo -E umount $(SD_DEV)2 || { echo -e "$(RED)[ERROR]$(NC) Failed to unmount $(SD_DEV)2."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Cleaning up temporary directories."
	@rm -rf boot rootfs
	@echo -e "$(GREEN)[SUCCESS]$(NC) SD card preparation completed."

# Prepare SD card
sd-raspi: $(ROOTFSDISK)
	@echo -e "$(BLUE)[INFO]$(NC) Unmounting $(SD_DEV)1 and $(SD_DEV)2 if mounted."
	@sudo -E umount $(SD_DEV)1 2>/dev/null || true
	@sudo -E umount $(SD_DEV)2 2>/dev/null || true
	@echo -e "$(BLUE)[INFO]$(NC) Setting up temporary directories."
	@rm -rf boot rootfs
	@mkdir -p boot rootfs || { echo -e "$(RED)[ERROR]$(NC) Failed to create boot/rootfs directories."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Decompressing $(ROOTFSDISK).xz."
	@xz -k -f -d "$(ROOTFSDISK).xz" || { echo -e "$(RED)[ERROR]$(NC) Failed to decompress $(ROOTFSDISK).xz."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Writing image to $(SD_DEV)."
	@sudo -E dd if=$(ROOTFSDISK) of=$(SD_DEV) bs=32M status=progress || { echo -e "$(RED)[ERROR]$(NC) Failed to write image to $(SD_DEV)."; exit 1; }
	@sync || { echo -e "$(RED)[ERROR]$(NC) Sync failed after dd."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Mounting $(SD_DEV)1 and $(SD_DEV)2."
	@sudo -E mount $(SD_DEV)1 boot || { echo -e "$(RED)[ERROR]$(NC) Failed to mount $(SD_DEV)1."; exit 1; }
	@sudo -E mount $(SD_DEV)2 rootfs || { echo -e "$(RED)[ERROR]$(NC) Failed to mount $(SD_DEV)2."; exit 1; }
	@sudo -E cp -f configs/config.txt boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy config.txt."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Creating user $(USER_NAME) with password."
	@echo "$(USER_NAME):$$(openssl passwd -6 -stdin <<< '$(PASSWD)')" > userconf.txt || { echo -e "$(RED)[ERROR]$(NC) Failed to create userconf.txt."; exit 1; }
	@sudo -E cp -f userconf.txt boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy userconf.txt."; exit 1; }
	@rm -f userconf.txt
	@echo -e "$(BLUE)[INFO]$(NC) Copying ssh boot script."
	@sudo -E cp -f configs/ssh boot/ || { echo -e "$(RED)[ERROR]$(NC) Failed to copy ssh."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Syncing and unmounting."
	@sync || { echo -e "$(RED)[ERROR]$(NC) Sync failed."; exit 1; }
	@sudo -E umount $(SD_DEV)1 || { echo -e "$(RED)[ERROR]$(NC) Failed to unmount $(SD_DEV)1."; exit 1; }
	@sudo -E umount $(SD_DEV)2 || { echo -e "$(RED)[ERROR]$(NC) Failed to unmount $(SD_DEV)2."; exit 1; }
	@echo -e "$(BLUE)[INFO]$(NC) Cleaning up temporary directories."
	@rm -rf boot rootfs
	@echo -e "$(GREEN)[SUCCESS]$(NC) SD card preparation completed. default user: [$(USER_NAME)], passwd: [$(PASSWD)]"

# Clean build artifacts
clean:
	@echo -e "$(BLUE)[INFO]$(NC) Cleaning build artifacts."
	@rm -rf $(BOOT_SCR) ramdisk.cpio.gz $(ROOTFSDISK).xz bcm2711-rpi-4-b.dtb $(KERNEL) $(RAMDISK) keys/dev.* || true
	@cd u-boot && make clean || true
	@cd linux && make ARCH=arm64 CROSS_COMPILE=${CROSS_PREFIX} clean || true
	@cd arm-trusted-firmware && make clean || true
	@echo -e "$(GREEN)[SUCCESS]$(NC) Clean completed."

# Clean everything, including submodules and downloads
distclean: clean
	@echo -e "$(BLUE)[INFO]$(NC) Performing deep clean."
	@rm -rf rootfs boot linux u-boot firmware rpi-buildroot arm-trusted-firmware $(ROOTFSDISK) || true
	@echo -e "$(GREEN)[SUCCESS]$(NC) Deep clean completed."

# Dependencies
$(BL31): tfa
$(KERNEL_IMAGE): linux
$(DTB_IMAGE): linux
$(RAMDISK): ramdisk
$(ROOTFSDISK): rootfs
$(BOOT_SCR): bootscr
$(KERNEL): fit
$(UBOOT): u-boot