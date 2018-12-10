/*
 * Copyright (C) 2004-2007 Atmel Corporation
 *
 * Based on MIPS implementation arch/mips/kernel/time.c
 *   Copyright 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*#define DEBUG*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/export.h>

static struct clk *cpuclk;

static int at32_verify_speed(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);
	return 0;
}

static unsigned int at32_get_speed(unsigned int cpu)
{
	/* No SMP support */
	if (cpu)
		return 0;
	return (unsigned int)((clk_get_rate(cpuclk) + 500) / 1000);
}

static unsigned int	ref_freq;
static unsigned long	loops_per_jiffy_ref;

static int at32_set_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned int old_freq, new_freq;

	old_freq = at32_get_speed(0);
	new_freq = freq_table[index].frequency;

	if (!ref_freq) {
		ref_freq = old_freq;
		loops_per_jiffy_ref = boot_cpu_data.loops_per_jiffy;
	}

	if (old_freq < new_freq)
		boot_cpu_data.loops_per_jiffy = cpufreq_scale(
				loops_per_jiffy_ref, ref_freq, new_freq);
	clk_set_rate(cpuclk, new_freq * 1000);
	if (new_freq < old_freq)
		boot_cpu_data.loops_per_jiffy = cpufreq_scale(
				loops_per_jiffy_ref, ref_freq, new_freq);

	return 0;
}

static int __init at32_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	cpuclk = clk_get(NULL, "cpu");
	if (IS_ERR(cpuclk)) {
		pr_debug("cpufreq: could not get CPU clk\n");
		return PTR_ERR(cpuclk);
	}

	policy->cpuinfo.min_freq = (clk_round_rate(cpuclk, 1) + 500) / 1000;
	policy->cpuinfo.max_freq = (clk_round_rate(cpuclk, ~0UL) + 500) / 1000;
	policy->cpuinfo.transition_latency = 0;
	policy->cur = at32_get_speed(0);
	policy->min = policy->cpuinfo.min_freq;
	policy->max = policy->cpuinfo.max_freq;

	printk("cpufreq: AT32AP CPU frequency driver\n");

	return 0;
}

static struct cpufreq_driver at32_driver = {
	.name		= "at32ap",
	.init		= at32_cpufreq_driver_init,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= at32_set_target,
	.get		= at32_get_speed,
	.flags		= CPUFREQ_STICKY,
};

static int __init at32_cpufreq_init(void)
{
	return cpufreq_register_driver(&at32_driver);
}
late_initcall(at32_cpufreq_init);
