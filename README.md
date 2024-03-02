# 1. Boot流程

以下是树莓派的boot流程：

![](https://raw.githubusercontent.com/carloscn/images/main/typora202403021452499.png)

1. SoC BootROM，这一阶段非常简单，主要支持读取SD中FAT32文件系统的第二阶段启动程序；
2. SoC BootROM加载并执行启动分区中的`bootcode.bin`，该文件主要功能是解析elf格式文件，并加载并解析同样位于分区中的`start4.elf`；
3. 运行`start4.elf`，读取并解析`config.txt`配置文件，并加载执行真正的u-boot程序。

# 2. SD卡分区

SD卡分区可以参考：

https://data.engrie.be/RaspberryPi/Raspberry_Pi_-_Part_16_-_Devices_Partitions_and_Filesystems_on_Your_Pi.pdf

![](https://raw.githubusercontent.com/carloscn/images/main/typora202403021500013.png)

我的分区是这样的：

![](https://raw.githubusercontent.com/carloscn/images/main/typora202403021504540.png)

## 2.1 boot分区

相关的boot分区在： https://github.com/raspberrypi/firmware 的boot中。里面包含了设备树、config.txt等文件、还有预编译的kernel文件。

由于u-boot中没有预置rpi4的dts文件（device tree source），因此采用了在u-boot运行时动态传入硬件描述dtb(device tree blob)文件的方式，用于u-boot启动时枚举硬件。

另外，在rpi3上运行过64位u-boot的都知道，如果在`config.txt`中没有特别指明kernel的位置，那么`start.elf`（或`start4.elf`）默认需要并启动的文件是`kernel8.img`：

1. `kernel8.img`：64位的Raspberry Pi 4和Raspberry Pi 4；
2. `kernel7l.img`：32位的Raspberry Pi 4（使用LPAE）；
3. `kernel7.img`：32位的Raspberry Pi 4、Raspberry Pi 3和Raspberry Pi 2（未使用LPAE）；
4. `kernel.img`：其他版本的树莓派。

## 2.2 rootfs分区

我们使 https://downloads.raspberrypi.com/raspios_lite_arm64/images/ 树莓派提供的img来做rootfs分区。需要注意的是，这个img文件，包含了boot分区和rootfs分区，我们可以把boot分区的内容删除，然后自己填充kernel和uboot。

另外还要注意，树莓派的用用户登录不再是默认的pi和raspberry了，而是需要把密码放在userconf.txt文件中，并且需要openssl加密。参考：

`sudo echo "carlos:$(openssl passwd -6 -stdin <<<'123456')" > boot/userconf.txt`

# 3. bsp编译

本仓库已经把所有的kernel和uboot以及firmware都准备好了，直接可以`make prepare` -> `make image`。

## 3.1 编译uboot

在树莓派中需要启动FIT格式的和FIT验签的功能，树莓派是默认没有的。已经在bsp/configs 提供了rpi_4_user_defconfig 用于支持FIT格式的image。

`make u-boot` 会自动编译uboot。

## 3.2 编译kernel

kernel不需要变动，直接`make linux`，会生成Image和Image.gz。

## 3.3 制作FIT格式

`make fit`

没有ramdisk的FIT image `make fit_noram`

# 4. uboot调试命令

```
fatload mmc 0:1 $fdt_addr_r bcm2711-rpi-4-b.dtb
fatload mmc 0:1 $kernel_addr_r Image
booti $kernel_addr_r - $fdt_addr_r

fatload mmc 0:1 4000000 image.ub
bootm 4000000
```