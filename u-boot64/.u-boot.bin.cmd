cmd_u-boot.bin := aarch64-openwrt-linux-gnu-objcopy  -j .text -j .rodata -j .data -j .u_boot_list -j .rela.dyn --gap-fill=0xff -O binary  u-boot u-boot.bin
