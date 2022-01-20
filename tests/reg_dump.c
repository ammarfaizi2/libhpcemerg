// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 *
 * Simple test for register dump.
 */
#include <hpcemerg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static struct hpcemerg_ctx *hpcemerg_ctx = NULL;

static const char expected_output[] =
	"  RIP: 0000000000401519\n"
	"  Code: c6 0b 00 00 00 49 c7 c7 0c 00 00 00 48 c7 c5 0d 00 00 00 48 c7 c3 0e 00 00 00 48 c7 c4 0f 00 00 00 0f 0b 48 8d 05 c7 28 00 00 <48> 0f 1f 00 48 0f 1f 00 48 0f 1f 00 48 0f 1f 00 48 0f 1f 00 48 0f \n"
	"  RSP: 000000000000000f EFLAGS: 00010246\n"
	"  RAX: 0000000000000000 RBX: 000000000000000e RCX: 0000000000000004\n"
	"  RDX: 0000000000000003 RSI: 0000000000000002 RDI: 0000000000000001\n"
	"  RBP: 000000000000000d R08: 0000000000000005 R09: 0000000000000006\n"
	"  R10: 0000000000000007 R11: 0000000000000008 R12: 0000000000000009\n"
	"  R13: 000000000000000a R14: 000000000000000b R15: 000000000000000c\n"
	"  CS: 0033 GS: 0000 FS: 0000 SS: 002b\n"
	"  CR2: 0000000000000000\n";

static char test_buf[4096];

static void handler(struct hpcemerg_sig_ctx *sig_ctx)
{
	int ret = 1;

	if (sig_ctx->sig != SIGILL) {
		/*
		 * ud2 generates SIGILL.
		 */
		fprintf(stderr, "sig != SIGILL\n");
		goto out;
	}

	if (sig_ctx->trap_data == NULL) {
		/*
		 * WARN*() must set trap_data.
		 */
		fprintf(stderr, "sig_ctx->trap_data == NULL\n");
		exit(1);
	}

	if (sig_ctx->trap_data->type != HPCEMERG_TRAP_WARN) {
		fprintf(stderr,
			"sig_ctx->trap_data.type != HPCEMERG_TRAP_WARN)\n");
		exit(1);
	}

	hpcemerg_register_dump(test_buf, sizeof(test_buf), sig_ctx);

	/* +226 skip the RIP and code, because they're always different! */
	if (strcmp(expected_output + 226, test_buf + 226)) {
		fprintf(stderr, "expected_output != buf\n");
		puts("====================================================");
		puts(expected_output + 226);
		puts("====================================================");
		puts(test_buf + 226);
		puts("====================================================");
		exit(1);
	}

	ret = 0;
out:
	exit(ret);
}

#if defined(__x86_64__)

#define BIG_ASM_NOP		\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"	\
	"nopq	(%%rax)\n\t"

#define BIG_ASM_NOP2	\
	BIG_ASM_NOP	\
	BIG_ASM_NOP	\
	BIG_ASM_NOP	\
	BIG_ASM_NOP	\
	BIG_ASM_NOP

__attribute__((__noinline__))
static void do_trap(void)
{
	static const struct hpcemerg_trap_data __trap_data = {
		.file = __FILE__,
		.func = __func__,
		.line = (uint32_t) __LINE__,
		.type = (HPCEMERG_TRAP_WARN)
	};

	__asm__ volatile(
		"# HPCEMERG_SET_TRAP\n\t"
		BIG_ASM_NOP2
		"movq	$0x00, %%rax\n\t"
		"movq	$0x01, %%rdi\n\t"
		"movq	$0x02, %%rsi\n\t"
		"movq	$0x03, %%rdx\n\t"
		"movq	$0x04, %%rcx\n\t"
		"movq	$0x05, %%r8\n\t"
		"movq	$0x06, %%r9\n\t"
		"movq	$0x07, %%r10\n\t"
		"movq	$0x08, %%r11\n\t"
		"movq	$0x09, %%r12\n\t"
		"movq	$0x0a, %%r13\n\t"
		"movq	$0x0b, %%r14\n\t"
		"movq	$0x0c, %%r15\n\t"
		"movq	$0x0d, %%rbp\n\t"
		"movq	$0x0e, %%rbx\n\t"
		"movq	$0x0f, %%rsp\n\t"
		"ud2\n\t"
		"leaq	%[__trap_data], %%rax\n\t"
		BIG_ASM_NOP2
		:
		: [__trap_data]"m"(__trap_data)
		: "memory"
	);
	__builtin_unreachable();
}
#endif /* #if defined(__x86_64__) */

int main(void)
{
	int ret;
	struct hpcemerg_init_param p = {
		.user_func	= &handler,
		.handle_bits	= HPCEMERG_TRAP_WARN
	};


	ret = hpcemerg_init(&p, &hpcemerg_ctx);
	if (ret) {
		errno = -ret;
		perror("hpcemerg_init");
		return -ret;
	}

#if defined(__x86_64__)
	do_trap();
	// Shouldn't be reached, we exit from the handler.
	return 1;
#else
	return 0;
#endif
}
