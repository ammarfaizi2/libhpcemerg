// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "../emerg.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

volatile short __emerg_taint = 0;
volatile emerg_callback_t __pre_emerg_print_trace = NULL;
volatile emerg_callback_t __post_emerg_print_trace = NULL;

static void __print_warn(const char *file, int line, const char *func)
{
	pr_intr("  WARNING: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


static void __print_bug(const char *file, int line, const char *func)
{
	pr_intr("  BUG: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


static bool is_ud2(void *rip)
{
	unsigned short ud2;
	memcpy(&ud2, rip, sizeof(ud2));
	return (ud2 == INSN_UD2);
}


static void dump_code(char *buf, unsigned long addr)
{
	unsigned char *end, *start = (unsigned char *)addr;
	start -= 30;
	end = start + 64;
	while (start < end) {
		buf += sprintf(
			buf,
			(start == (unsigned char *)addr) ? "<%02x> " : "%02x ",
			(unsigned)*start
		);
		start++;
	}
}


static void register_dump_print(unsigned long *gregs)
{
	char code_buf[512];
	unsigned short cgfs[4]; /* unsigned short cs, gs, fs, ss.  */
	memcpy(cgfs, &gregs[REG_CSGSFS], sizeof(cgfs));
	dump_code(code_buf, gregs[REG_RIP]);
	void *rip = (void *)gregs[REG_RIP];
	char **rip_strp;

	rip_strp = backtrace_symbols(&rip, 1);
	pr_intr(
		"  RIP: %016lx %s\n"
		"  Code: %s\n"
		"  RSP: %016lx EFLAGS: %08lx\n"
		"  RAX: %016lx RBX: %016lx RCX: %016lx\n"
		"  RDX: %016lx RSI: %016lx RDI: %016lx\n"
		"  RBP: %016lx R08: %016lx R09: %016lx\n"
		"  R10: %016lx R11: %016lx R12: %016lx\n"
		"  R13: %016lx R14: %016lx R15: %016lx\n"
		"  CS: %04x GS: %04x FS: %04x SS: %04x\n"
		"  CR2: %016lx\n\n",
		gregs[REG_RIP], (rip_strp ? *rip_strp : ""),
		code_buf,
		gregs[REG_RSP], gregs[REG_EFL],
		gregs[REG_RAX], gregs[REG_RBX], gregs[REG_RCX],
		gregs[REG_RDX], gregs[REG_RSI], gregs[REG_RDI],
		gregs[REG_RBP], gregs[REG_R8], gregs[REG_R9],
		gregs[REG_R10], gregs[REG_R11], gregs[REG_R12],
		gregs[REG_R13], gregs[REG_R14], gregs[REG_R15],
			/* cs */ cgfs[0],
			/* gs */ cgfs[1],
			/* fs */ cgfs[2],
			/* ss */ cgfs[3],
		gregs[REG_CR2]
	);
	free(rip_strp);
}


static struct emerg_entry____ *get_entry(void *_rip)
{
	int disp;
	uintptr_t orig_rip = (uintptr_t)_rip;
	uintptr_t rip = orig_rip;

	rip += 2; /* Skip UD2 */
	rip += 1; /* Skip REX prefix */

	/* Is a LEA r64, m? */
	if (memcmp((void *)rip, "\x8d\x05", 2))
		return NULL;

	memcpy(&disp, (void *)(rip + 2), sizeof(disp));
	orig_rip += 2 + 7;
	orig_rip += disp;
	return (struct emerg_entry____ *)orig_rip;
}


static void print_backtrace(uintptr_t rip)
{
	int nptrs, i;
	uintptr_t buf[500];
	char **strings;

	nptrs = backtrace((void **)buf, sizeof(buf) / sizeof(buf[0]));
	if (nptrs <= 0)
		return;

	for (i = 0; i < nptrs; i++) {
		Dl_info ii;
		const char *func, *file;
		unsigned func_off = 0;

		if (!dladdr((void *)buf[i], &ii))
			continue;

		func = ii.dli_sname;
		file = ii.dli_fname;
		if (func)
			func_off = buf[i] - (uintptr_t)ii.dli_saddr;
		else
			func = "????????";

		pr_intr("  %s%#016lx %s+%#x [%s]\n",
			(rip == (uintptr_t)buf[i]) ? "%rip => " : "        ",
			buf[i], func, func_off, file
		);
	}
}


static void _emerg_print_trace(int sig, siginfo_t *si, void *arg)
{
	ucontext_t *ctx = (ucontext_t *)arg;
	mcontext_t *mctx = &ctx->uc_mcontext;
	unsigned long *gregs = (unsigned long *)&mctx->gregs;
	void *rip = (void *)gregs[REG_RIP];
	struct emerg_entry____ *entry;

	if (!is_ud2(rip))
		abort();

	entry = get_entry(rip);
	if (!entry)
		abort();

	__emerg_taint |= entry->taint;
	switch (entry->emerg_type) {
	case EMERG_TYPE_BUG:
		__print_bug(entry->file, entry->line, entry->func);
		break;
	case EMERG_TYPE_WARN:
		__print_warn(entry->file, entry->line, entry->func);
		break;
	}
	pr_intr("  %s: %d", __emerg_taint ? "Tainted" : "Not tainted",
		__emerg_taint);
	pr_intr("; Compiler: " __VERSION__ "\n");
	register_dump_print(gregs);
	puts("  Call trace: ");
	print_backtrace((uintptr_t)rip);
	gregs[REG_RIP] += 2 + 7;
}


void dump_stack(int sig, siginfo_t *si, void *arg)
{
	emerg_callback_t pre_emerg_print_trace, post_emerg_print_trace;

	pr_intr("==========================================================\n");
	pre_emerg_print_trace = __pre_emerg_print_trace;
	if (!pre_emerg_print_trace || pre_emerg_print_trace(sig, si, arg))
		_emerg_print_trace(sig, si, arg);

	post_emerg_print_trace = __post_emerg_print_trace;
	if (post_emerg_print_trace)
		post_emerg_print_trace(sig, si, arg);
	pr_intr("==========================================================\n");
}


void emerg_print_trace_init(void)
{
	struct sigaction action;
	action.sa_sigaction = &dump_stack;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGILL, &action, NULL);
}
