# Remote-builder entry. `make ci` does not write an SD card.
# Hardware programming stays on `make hw-sd` and needs SD_DEV.

.PHONY: help ci config-check build test package clean hw-sd

help:
	@echo "make ci      check, build, test, package into \$$RB_OUT"
	@echo "make hw-sd   write an SD card (not called by ci); set SD_DEV"

ci: config-check build test package

config-check:
	@test -n "$(RB_OUT)" || { echo "RB_OUT is not set"; exit 1; }
	@mkdir -p "$(RB_OUT)"
	@$(MAKE) -C bsp config-check

build:
	$(MAKE) -C bsp prepare
	$(MAKE) -C bsp image

test:
	@test -f bsp/image.ub
	@test -f bsp/u-boot/u-boot.bin
	@test -f bsp/bcm2711-rpi-4-b.dtb
	@test -f bsp/boot.scr
	@test -f bsp/arm-trusted-firmware/build/rpi4/debug/bl31.bin
	@bsp/u-boot/tools/fit_check_sign -f bsp/image.ub -k bsp/bcm2711-rpi-4-b.dtb

package:
	@test -n "$(RB_OUT)" || { echo "RB_OUT is not set"; exit 1; }
	@mkdir -p "$(RB_OUT)"
	cp -f bsp/u-boot/u-boot.bin "$(RB_OUT)/u-boot.bin"
	cp -f bsp/image.ub "$(RB_OUT)/image.ub"
	cp -f bsp/boot.scr "$(RB_OUT)/boot.scr"
	cp -f bsp/bcm2711-rpi-4-b.dtb "$(RB_OUT)/bcm2711-rpi-4-b.dtb"
	cp -f bsp/arm-trusted-firmware/build/rpi4/debug/bl31.bin "$(RB_OUT)/bl31.bin"
	printf '%s\n' '{"artifacts":["u-boot.bin","image.ub","boot.scr","bcm2711-rpi-4-b.dtb","bl31.bin"]}' > "$(RB_OUT)/rb-result.json"

hw-sd:
	$(MAKE) -C bsp sd

clean:
	$(MAKE) -C bsp clean
