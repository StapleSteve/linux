/*
 *	linux/arch/arm/mach-nspire/nspire.c
 *
 *	Copyright (C) 2012 Daniel Tang <tangrs@tangrs.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/irqchip.h>
#include <linux/irqchip/arm-vic.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>

#include <clocksource/nspire_classic_timer.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/hardware/timer-sp.h>

#include "mmio.h"
#include "clcd.h"

static const char *nspire_dt_match[] __initconst = {
	"arm,nspire",
	"arm,nspire-cx",
	"arm,nspire-tp",
	"arm,nspire-clp",
	NULL,
};

static struct map_desc nspire_io_desc[] __initdata = {
	{
		.virtual	=  NSPIRE_EARLY_UART_VIRT_BASE,
		.pfn		= __phys_to_pfn(NSPIRE_EARLY_UART_PHYS_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}
};

#define TIMER_NAME_MAXLEN 32
static void __init nspire_add_sp804_timers(void)
{
	struct device_node *of_timer;
	int clockevents_found = 0;

	for_each_compatible_node(of_timer, NULL, "arm,sp804") {
		struct resource res;
		void __iomem *base;
		char *timer_name;
		struct clk *clk;
		int irq;

		clk = of_clk_get_by_name(of_timer, NULL);
		if (WARN_ON(!clk))
			continue;

		timer_name = kzalloc(TIMER_NAME_MAXLEN, GFP_ATOMIC);
		if (!timer_name)
			break;

		base = of_iomap(of_timer, 0);
		if (WARN_ON(!base)) {
			kfree(timer_name);
			continue;
		}

		of_address_to_resource(of_timer, 0, &res);
		scnprintf(timer_name, TIMER_NAME_MAXLEN, "%llx.%s",
				(unsigned long long)res.start,
				of_timer->name);

		clk_register_clkdev(clk, timer_name, "sp804");
		if (!clockevents_found) {
			irq = irq_of_parse_and_map(of_timer, 0);
			if (irq) {
				sp804_clockevents_init(base, irq,timer_name);
				clockevents_found = 1;
				continue;
			}
		}

		sp804_clocksource_init(base, timer_name);
	}
}


static void __init nspire_init_timer(void)
{
	of_clk_init(NULL);
	nspire_classic_timer_init();
	nspire_add_sp804_timers();
}

static void __init nspire_map_io(void)
{
	iotable_init(nspire_io_desc, ARRAY_SIZE(nspire_io_desc));
}

static struct clcd_board nspire_clcd_data = {
	.name		= "LCD",
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.setup		= nspire_clcd_setup,
	.mmap		= nspire_clcd_mmap,
	.remove		= nspire_clcd_remove,
};


static struct of_dev_auxdata nspire_auxdata[] __initdata = {
	OF_DEV_AUXDATA("arm,pl111", NSPIRE_LCD_PHYS_BASE,
			NULL, &nspire_clcd_data),
	{ }
};

static void __init nspire_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table,
			nspire_auxdata, NULL);
}

static void nspire_restart(char mode, const char *cmd)
{
}

DT_MACHINE_START(NSPIRE, "TI-NSPIRE")
	.map_io		= nspire_map_io,
	.init_irq	= irqchip_init,
	.init_time	= nspire_init_timer,
	.init_machine	= nspire_init,
	.dt_compat	= nspire_dt_match,
	.restart	= nspire_restart,
MACHINE_END
