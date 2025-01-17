// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2009
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
 *
 * Copyright (C) 2011
 * Stefano Babic, DENX Software Engineering, <sbabic@denx.de>
 *
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 */
#include <common.h>
#include <errno.h>
#include <dm.h>
#include <malloc.h>
#include <asm/arch/imx-regs.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <errno.h>
#ifdef CONFIG_IMX_RDC
#include <asm/mach-imx/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#endif

enum mxc_gpio_direction {
	MXC_GPIO_DIRECTION_IN,
	MXC_GPIO_DIRECTION_OUT,
};

#define GPIO_PER_BANK			32

struct mxc_gpio_plat {
	int bank_index;
	struct gpio_regs *regs;
};

struct mxc_bank_info {
	struct gpio_regs *regs;
};

#if !CONFIG_IS_ENABLED(DM_GPIO)
#define GPIO_TO_PORT(n)		((n) / 32)

/* GPIO port description */
static unsigned long gpio_ports[] = {
	[0] = GPIO1_BASE_ADDR,
	[1] = GPIO2_BASE_ADDR,
	[2] = GPIO3_BASE_ADDR,
#if defined(CONFIG_MX25) || defined(CONFIG_MX27) || defined(CONFIG_MX51) || \
		defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_MX7) || defined(CONFIG_IMX8M) || \
		defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMXRT1050)
	[3] = GPIO4_BASE_ADDR,
#endif
#if defined(CONFIG_MX27) || defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_MX7) || defined(CONFIG_IMX8M) || \
		defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMXRT1050)
	[4] = GPIO5_BASE_ADDR,
#if !(defined(CONFIG_MX6UL) || defined(CONFIG_MX6ULL) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_IMXRT1050))
	[5] = GPIO6_BASE_ADDR,
#endif
#endif
#if defined(CONFIG_MX53) || defined(CONFIG_MX6) || defined(CONFIG_MX7) || \
		defined(CONFIG_ARCH_IMX8)
#if !(defined(CONFIG_MX6UL) || defined(CONFIG_MX6ULL))
	[6] = GPIO7_BASE_ADDR,
#endif
#endif
#if defined(CONFIG_ARCH_IMX8)
	[7] = GPIO8_BASE_ADDR,
#endif
};

#ifdef CONFIG_IMX_RDC
static unsigned int gpio_rdc[] = {
	RDC_PER_GPIO1,
	RDC_PER_GPIO2,
	RDC_PER_GPIO3,
	RDC_PER_GPIO4,
	RDC_PER_GPIO5,
	RDC_PER_GPIO6,
	RDC_PER_GPIO7,
};

#define RDC_CHECK(x) imx_rdc_check_permission(gpio_rdc[x], 0)
#define RDC_SPINLOCK_UP(x) imx_rdc_sema_lock(gpio_rdc[x])
#define RDC_SPINLOCK_DOWN(x) imx_rdc_sema_unlock(gpio_rdc[x])
#else
#define RDC_CHECK(x) 0
#define RDC_SPINLOCK_UP(x)
#define RDC_SPINLOCK_DOWN(x)
#endif


static int mxc_gpio_direction(unsigned int gpio,
	enum mxc_gpio_direction direction)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 l;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	l = readl(&regs->gpio_dir);

	switch (direction) {
	case MXC_GPIO_DIRECTION_OUT:
		l |= 1 << gpio;
		break;
	case MXC_GPIO_DIRECTION_IN:
		l &= ~(1 << gpio);
	}
	writel(l, &regs->gpio_dir);

	RDC_SPINLOCK_DOWN(port);

	return 0;
}

int gpio_set_value(unsigned gpio, int value)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 l;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	l = readl(&regs->gpio_dr);
	if (value)
		l |= 1 << gpio;
	else
		l &= ~(1 << gpio);
	writel(l, &regs->gpio_dr);

	RDC_SPINLOCK_DOWN(port);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	struct gpio_regs *regs;
	u32 val;

	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	RDC_SPINLOCK_UP(port);

	gpio &= 0x1f;

	regs = (struct gpio_regs *)gpio_ports[port];

	val = (readl(&regs->gpio_dr) >> gpio) & 0x01;

	RDC_SPINLOCK_DOWN(port);

	return val;
}

int gpio_request(unsigned gpio, const char *label)
{
	unsigned int port = GPIO_TO_PORT(gpio);
	if (port >= ARRAY_SIZE(gpio_ports))
		return -1;

	if (RDC_CHECK(port))
		return -1;

	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	return mxc_gpio_direction(gpio, MXC_GPIO_DIRECTION_IN);
}

int gpio_direction_output(unsigned gpio, int value)
{
	int ret = gpio_set_value(gpio, value);

	if (ret < 0)
		return ret;

	return mxc_gpio_direction(gpio, MXC_GPIO_DIRECTION_OUT);
}
#endif

#if CONFIG_IS_ENABLED(DM_GPIO)
#include <fdtdec.h>
static int mxc_gpio_is_output(struct gpio_regs *regs, int offset)
{
	u32 val;

	val = readl(&regs->gpio_dir);

	return val & (1 << offset) ? 1 : 0;
}

static void mxc_gpio_bank_direction(struct gpio_regs *regs, int offset,
				    enum mxc_gpio_direction direction)
{
	u32 l;

	l = readl(&regs->gpio_dir);

	switch (direction) {
	case MXC_GPIO_DIRECTION_OUT:
		l |= 1 << offset;
		break;
	case MXC_GPIO_DIRECTION_IN:
		l &= ~(1 << offset);
	}
	writel(l, &regs->gpio_dir);
}

static void mxc_gpio_bank_set_value(struct gpio_regs *regs, int offset,
				    int value)
{
	u32 l;

	l = readl(&regs->gpio_dr);
	if (value)
		l |= 1 << offset;
	else
		l &= ~(1 << offset);
	writel(l, &regs->gpio_dr);
}

static int mxc_gpio_bank_get_value(struct gpio_regs *regs, int offset)
{
	return (readl(&regs->gpio_dr) >> offset) & 0x01;
}

/* set GPIO pin 'gpio' as an input */
static int mxc_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);

	/* Configure GPIO direction as input. */
	mxc_gpio_bank_direction(bank->regs, offset, MXC_GPIO_DIRECTION_IN);

	return 0;
}

/* set GPIO pin 'gpio' as an output, with polarity 'value' */
static int mxc_gpio_direction_output(struct udevice *dev, unsigned offset,
				       int value)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);

	/* Configure GPIO output value. */
	mxc_gpio_bank_set_value(bank->regs, offset, value);

	/* Configure GPIO direction as output. */
	mxc_gpio_bank_direction(bank->regs, offset, MXC_GPIO_DIRECTION_OUT);

	return 0;
}

/* read GPIO IN value of pin 'gpio' */
static int mxc_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);

	return mxc_gpio_bank_get_value(bank->regs, offset);
}

/* write GPIO OUT value to pin 'gpio' */
static int mxc_gpio_set_value(struct udevice *dev, unsigned offset,
				 int value)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);

	mxc_gpio_bank_set_value(bank->regs, offset, value);

	return 0;
}

static int mxc_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);

	/* GPIOF_FUNC is not implemented yet */
	if (mxc_gpio_is_output(bank->regs, offset))
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static const struct dm_gpio_ops gpio_mxc_ops = {
	.direction_input	= mxc_gpio_direction_input,
	.direction_output	= mxc_gpio_direction_output,
	.get_value		= mxc_gpio_get_value,
	.set_value		= mxc_gpio_set_value,
	.get_function		= mxc_gpio_get_function,
};

static int mxc_gpio_probe(struct udevice *dev)
{
	struct mxc_bank_info *bank = dev_get_priv(dev);
	struct mxc_gpio_plat *plat = dev_get_platdata(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	int banknum;
	char name[18], *str;

	banknum = plat->bank_index;
#if defined(CONFIG_ARCH_IMX8)
	sprintf(name, "GPIO%d_", banknum);
#else
	sprintf(name, "GPIO%d_", banknum + 1);
#endif
	str = strdup(name);
	if (!str)
		return -ENOMEM;
	uc_priv->bank_name = str;
	uc_priv->gpio_count = GPIO_PER_BANK;
	bank->regs = plat->regs;

	return 0;
}

static int mxc_gpio_ofdata_to_platdata(struct udevice *dev)
{
	fdt_addr_t addr;
	struct mxc_gpio_plat *plat = dev_get_platdata(dev);

	addr = devfdt_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	plat->regs = (struct gpio_regs *)addr;
	plat->bank_index = dev->req_seq;

	return 0;
}

static int mxc_gpio_bind(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id mxc_gpio_ids[] = {
	{ .compatible = "fsl,imx35-gpio" },
	{ }
};

U_BOOT_DRIVER(gpio_mxc) = {
	.name	= "gpio_mxc",
	.id	= UCLASS_GPIO,
	.ops	= &gpio_mxc_ops,
	.probe	= mxc_gpio_probe,
	.ofdata_to_platdata = mxc_gpio_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct mxc_gpio_plat),
	.priv_auto_alloc_size = sizeof(struct mxc_bank_info),
	.of_match = mxc_gpio_ids,
	.bind	= mxc_gpio_bind,
};

#if !CONFIG_IS_ENABLED(OF_CONTROL)
static const struct mxc_gpio_plat mxc_plat[] = {
	{ 0, (struct gpio_regs *)GPIO1_BASE_ADDR },
	{ 1, (struct gpio_regs *)GPIO2_BASE_ADDR },
	{ 2, (struct gpio_regs *)GPIO3_BASE_ADDR },
#if defined(CONFIG_MX25) || defined(CONFIG_MX27) || defined(CONFIG_MX51) || \
		defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_ARCH_IMX8)
	{ 3, (struct gpio_regs *)GPIO4_BASE_ADDR },
#endif
#if defined(CONFIG_MX27) || defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_ARCH_IMX8)
	{ 4, (struct gpio_regs *)GPIO5_BASE_ADDR },
#if !(defined(CONFIG_MX6UL) || defined(CONFIG_MX6ULL) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_IMXRT1050))
	{ 5, (struct gpio_regs *)GPIO6_BASE_ADDR },
#endif
#endif
#if defined(CONFIG_MX53) || defined(CONFIG_MX6) || defined(CONFIG_ARCH_IMX8)
#if !(defined(CONFIG_MX6UL) || defined(CONFIG_MX6ULL))
	{ 6, (struct gpio_regs *)GPIO7_BASE_ADDR },
#endif
#endif
#if defined(CONFIG_ARCH_IMX8)
	{ 7, (struct gpio_regs *)GPIO8_BASE_ADDR },
#endif
};

U_BOOT_DEVICES(mxc_gpios) = {
	{ "gpio_mxc", &mxc_plat[0] },
	{ "gpio_mxc", &mxc_plat[1] },
	{ "gpio_mxc", &mxc_plat[2] },
#if defined(CONFIG_MX25) || defined(CONFIG_MX27) || defined(CONFIG_MX51) || \
		defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_ARCH_IMX8)
	{ "gpio_mxc", &mxc_plat[3] },
#endif
#if defined(CONFIG_MX27) || defined(CONFIG_MX53) || defined(CONFIG_MX6) || \
		defined(CONFIG_IMX8M) || defined(CONFIG_ARCH_IMX8)
	{ "gpio_mxc", &mxc_plat[4] },
#ifndef CONFIG_IMX8M
	{ "gpio_mxc", &mxc_plat[5] },
#endif
#endif
#if defined(CONFIG_MX53) || defined(CONFIG_MX6) || defined(CONFIG_ARCH_IMX8)
	{ "gpio_mxc", &mxc_plat[6] },
#endif
#if defined(CONFIG_ARCH_IMX8)
	{ "gpio_mxc", &mxc_plat[7] },
#endif
};
#endif
#endif
