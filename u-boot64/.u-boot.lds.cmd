cmd_u-boot.lds := aarch64-openwrt-linux-gnu-gcc -E -Wp,-MD,./.u-boot.lds.d -D__KERNEL__ -D__UBOOT__ -DCONFIG_SYS_TEXT_BASE=0x00020000   -D__ARM__           -mstrict-align  -ffunction-sections -fdata-sections -fno-common -ffixed-r9   -fno-common -ffixed-x18 -pipe -march=armv8-a   -Iinclude  -I./arch/arm/include -include ./include/linux/kconfig.h  -nostdinc -isystem /work/rtk/sery/NVR_18/staging_dir/toolchain-aarch64_cortex-a53+neon_gcc-4.9-linaro_glibc-2.19/lib/gcc/aarch64-openwrt-linux-gnu/4.9.4/include -include ./include/u-boot/u-boot.lds.h -DCPUDIR=arch/arm/cpu/armv8 -DMIPS_BOOTLOAD_LIB_PATH=image/rtd1295/src/app/libbootload.o  -ansi -D__ASSEMBLY__ -x assembler-with-cpp -P -o u-boot.lds board/realtek/1295_qa/u-boot.lds

source_u-boot.lds := board/realtek/1295_qa/u-boot.lds

deps_u-boot.lds := \
  include/u-boot/u-boot.lds.h \

u-boot.lds: $(deps_u-boot.lds)

$(deps_u-boot.lds):
