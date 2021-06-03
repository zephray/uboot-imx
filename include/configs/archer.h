/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *
 * Configuration settings for the Archer board.
 */

#ifndef __MX7D_ARCHER_CONFIG_H
#define __MX7D_ARCHER_CONFIG_H

#include "mx7_common.h"

#define PHYS_SDRAM_SIZE			SZ_1G

#define CONFIG_MXC_UART_BASE            UART1_IPS_BASE_ADDR

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(32 * SZ_1M)

/* MMC Config*/
#define CONFIG_SYS_FSL_ESDHC_ADDR       0

/* I2C configs */
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_SPEED		100000

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#define UPDATE_M4_ENV ""
#define MFG_NAND_PARTITION ""

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG
#define CONFIG_FASTBOOT_USB_DEV 0

#define CONFIG_DFU_ENV_SETTINGS \
	"dfu_alt_info=image raw 0 0x800000;"\
		"u-boot raw 0 0x4000;"\
		"bootimg part 0 1;"\
		"rootfs part 0 2\0" \

#define CONFIG_MFG_ENV_SETTINGS \
	"bootcmd_mfg=run mmcargs;" \
	"if test ${tee} = yes; then " \
		"bootm ${tee_addr} ${initrdaddr} ${fdt_addr}; " \
	"else " \
		"bootz ${loadaddr} ${initrdaddr} ${fdt_addr}; " \
	"fi;\0" \

#define CONFIG_EXTRA_ENV_SETTINGS \
	UPDATE_M4_ENV \
	CONFIG_MFG_ENV_SETTINGS \
	TEE_ENV \
	CONFIG_DFU_ENV_SETTINGS \
	"image=boot/zImage\0" \
	"console=ttymxc0\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=boot/imx7d-archer.dtb\0" \
	"fdt_addr=0x83000000\0" \
	"tee_addr=0x84000000\0" \
	"initrdaddr=0x86800000\0" \
	"tee_file=uTee-7dsdb\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"splashimage=0x8c000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"earlycon=ec_imx6q,0x30860000,${baudrate} " \
		"epdc video=mxcepdcfb:ES103TC1,bpp=16 " \
		"root=${mmcroot}\0" \
	"loadimage=ext4load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=ext4load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"loadtee=ext4load mmc ${mmcdev}:${mmcpart} ${tee_addr} ${tee_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${tee} = yes; then " \
			"run loadfdt; run loadtee; bootm ${tee_addr} - ${fdt_addr}; " \
		"else " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if run loadfdt; then " \
					"bootz ${loadaddr} ${initrdaddr} ${fdt_addr}; " \
				"else " \
					"if test ${boot_fdt} = try; then " \
						"bootz; " \
					"else " \
						"echo WARN: Cannot load the DT; " \
					"fi; " \
				"fi; " \
			"else " \
				"bootz; " \
			"fi; " \
		"fi;\0" \

#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev};" \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
			"if run loadimage; then " \
				"run mmcboot; " \
			"else run netboot; " \
			"fi; " \
	   "else run netboot; fi"

#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x20000000)

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR
#define CONFIG_SYS_HZ			1000

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* environment organization */

#if defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED
#endif

#ifdef CONFIG_FSL_QSPI
#define CONFIG_SYS_AUXCORE_BOOTDATA 0x60100000 /* Set to QSPI1 A flash, offset 1M */
#else
#define CONFIG_SYS_AUXCORE_BOOTDATA 0x7F8000 /* Set to TCML address */
#endif

#define CONFIG_SYS_FSL_USDHC_NUM	1

/* MMC Config*/
#define CONFIG_SYS_FSL_ESDHC_ADDR       0
#define CONFIG_SYS_MMC_ENV_DEV		0   /* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p1"  /* USDHC1 */

/* USB Configs */
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)

#define CONFIG_IMX_THERMAL

/* #define CONFIG_SPLASH_SCREEN*/
/* #define CONFIG_MXC_EPDC*/

/*
 * SPLASH SCREEN Configs
 */
#if defined(CONFIG_MXC_EPDC)
/*
 * Framebuffer and LCD
 */
#define CONFIG_SPLASH_SCREEN

#undef LCD_TEST_PATTERN
/* #define CONFIG_SPLASH_IS_IN_MMC			1 */
#define LCD_BPP					LCD_MONOCHROME
/* #define CONFIG_SPLASH_SCREEN_ALIGN		1 */

#define CONFIG_WAVEFORM_BUF_SIZE		0x400000
#endif

#endif	/* __CONFIG_H */
