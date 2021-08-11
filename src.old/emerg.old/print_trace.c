// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define __USE_GNU
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>

#define IN_PRINT_TRACE_C
#include "print_trace.h"
#undef IN_PRINT_TRACE_C


static bool virt_addr_valid(unsigned long addr)
{
	(void)addr;
	return true;
}


static void dump_code(char *buf, unsigned long addr)
{
	unsigned char *end, *start = (unsigned char *)addr;
	start -= 30;
	end = start + 45;
	while (start < end) {
		buf += sprintf(
			buf,
			(start == (unsigned char *)addr) ? "<%02x> " : "%02x ",
			(unsigned)*start
		);
		start++;
	}
}


static unsigned emerg_type(unsigned char *rip)
{
	if (!memcmp(rip, WARN_ON_OPCODE, sizeof(WARN_ON_OPCODE) - 1))
		return EMERG_PR_WARN_ON;
	if (!memcmp(rip, BUG_ON_OPCODE, sizeof(BUG_ON_OPCODE) - 1))
		return EMERG_PR_BUG_ON;
	return EMERG_PR_UNKNOWN;
}


static void print_backtrace(void)
{
	int nptrs;
	void *buffer[300];
	char **strings;

	nptrs = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
	printf("backtrace() returned %d addresses\n", nptrs);

	/*
	 * The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	 * would produce similar output to the following:
	 */
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (int j = 0; j < nptrs; j++)
		printf("%s\n", strings[j]);

	free(strings);
}


void emerg_print_trace(int sig, siginfo_t *si, void *arg)
{
	(void)si;

	unsigned type;
	char code_buf[512];
	ucontext_t *ctx = (ucontext_t *)arg;
	mcontext_t *mctx = &ctx->uc_mcontext;
	unsigned long *gregs = (unsigned long *)&mctx->gregs;
	unsigned short cgfs[4]; /* unsigned short cs, gs, fs, ss.  */
	memcpy(cgfs, &gregs[REG_CSGSFS], sizeof(cgfs));

	if (virt_addr_valid(gregs[REG_RIP])) {
		dump_code(code_buf, gregs[REG_RIP]);
		type = emerg_type((unsigned char *)gregs[REG_RIP]);
	} else {
		type = EMERG_PR_UNKNOWN;
		strncpy(code_buf, "Invalid RIP", sizeof(code_buf));
	}

	pr_intr(
		"  RIP: %016lx\n"
		"  Code: %s\n"
		"  RSP: %016lx EFLAGS: %08lx\n"
		"  RAX: %016lx RBX: %016lx RCX: %016lx\n"
		"  RDX: %016lx RSI: %016lx RDI: %016lx\n"
		"  RBP: %016lx R08: %016lx R09: %016lx\n"
		"  R10: %016lx R11: %016lx R12: %016lx\n"
		"  R13: %016lx R14: %016lx R15: %016lx\n"
		"  CS: %04x GS: %04x FS: %04x SS: %04x\n"
		"  CR2: %016lx\n\n",
		gregs[REG_RIP],
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
	print_backtrace();

	switch (type) {
	case EMERG_PR_WARN_ON:
		gregs[REG_RIP] += sizeof(WARN_ON_OPCODE);
		break;
	case EMERG_PR_BUG_ON:
		gregs[REG_RIP] += sizeof(BUG_ON_OPCODE);
		break;
	default:
		abort();
	}
}


void __print_warn(const char *file, int line, const char *func)
{
	pr_intr("  WARNING: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


void __print_bug(const char *file, int line, const char *func)
{
	pr_intr("  BUG: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


void emerg_print_trace_init(void)
{
	struct sigaction action;
	action.sa_sigaction = &emerg_print_trace;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
}
