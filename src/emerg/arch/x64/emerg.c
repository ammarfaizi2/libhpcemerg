// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "../../emerg.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define EMERG_FILE_VALIDATOR "/tmp/qqq"

volatile short __emerg_taint = 0;
volatile emerg_callback_t __pre_emerg_print_trace = NULL;
volatile emerg_callback_t __post_emerg_print_trace = NULL;
static unsigned emerg_init_bits = 0;
static volatile int handler_lock = -1;
static int validator_fd = -1;

#define lock_cmpxchg(PTR, OLD, NEW) ({				\
	int __ret;						\
	int __old = (OLD);					\
	int __new = (NEW);					\
	volatile int *__ptr = (volatile int *)(PTR);		\
	__asm__ volatile(					\
		"lock\tcmpxchgl %2,%1"				\
		: "=a"(__ret), "+m" (*__ptr)			\
		: "r"(__new), "0"(__old)			\
		: "memory"					\
	);							\
	(__ret);						\
})

#define atomic_read(PTR) ({					\
	int __ret;						\
	volatile int *__ptr = (volatile int *)(PTR);		\
	__asm__ volatile(					\
		"movl %1, %0"					\
		: "=r"(__ret)					\
		: "m"(*__ptr)					\
		: "memory"					\
	);							\
	(__ret);						\
})

#define atomic_set(PTR, VAL) ({					\
	int __val = (VAL);					\
	volatile int *__ptr = (volatile int *)(PTR);		\
	__asm__ volatile(					\
		"movl %1, %0"					\
		: "=m"(*__ptr)					\
		: "r"(__val)					\
		: "memory"					\
	);							\
	(__val);						\
})

#define cpu_relax() __asm__ volatile("rep\n\tnop")


static int is_valid_addr(void *addr, size_t len)
{
	if (validator_fd == -1) {
		validator_fd = open(EMERG_FILE_VALIDATOR, O_CREAT | O_WRONLY, 0777);
		if (validator_fd < 0) {
			return 0;
		}
	}

	return (write(validator_fd, addr, len) == (ssize_t)len);
}


static int is_emerg_pattern(void *addr)
{
	/*
	 * "\x0f\x0b"  ud2
	 * "\x8d\x05"  lea rax, [rip + ???]
	 */
	return !memcmp("\x0f\x0b\x8d\x05", addr, 4);
}


static void dump_code(char *buf, void *addr)
{
	unsigned char *end, *start = (unsigned char *)addr;
	start -= 42;
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


struct func_addr {
	const char	*name;
	const char	*file;
	unsigned	rip_next;
};

static struct func_addr *func_addr_translate(struct func_addr *buf, void *addr)
{
	Dl_info i;
	if (!dladdr(addr, &i))
		return NULL;

	buf->name = i.dli_sname;
	buf->file = i.dli_fname;
	buf->rip_next = (uintptr_t)addr - (uintptr_t)i.dli_saddr;
	return buf;
}


static void print_backtrace(uintptr_t rip)
{
	int nptrs, i;
	uintptr_t buf[300];

	pr_intr("  Call Trace: \n");
	if (!is_valid_addr((void *)rip, 1)) {
		pr_intr("    Not printing call trace due to invalid RIP!\n"
			"    This is very likely not recoverable.\n"
			"    Please use the coredump file for further investigation!\n");
		return;
	}
	nptrs = backtrace((void **)buf, sizeof(buf) / sizeof(buf[0]));
	if (nptrs <= 0) {
		pr_intr("  Unable to retrieve backtrace!\n");
		return;
	}

	for (i = 0; i < nptrs; i++) {
		struct func_addr fx;
		uintptr_t fa = (uintptr_t)buf[i];

		if (!func_addr_translate(&fx, (void *)fa))
			continue;

		pr_intr("  %s[%#016lx] %s(%s+%#x)\n",
			(rip == fa) ? "%rip => " : "        ",
			fa,
			fx.file,
			fx.name ? fx.name : "??? 0",
			fx.rip_next
		);
	}
}

static void dump_register(uintptr_t *regs)
{
	char code_buf[512];
	unsigned short cgfs[4]; /* unsigned short cs, gs, fs, ss.  */
	memcpy(cgfs, &regs[REG_CSGSFS], sizeof(cgfs));
	void *rip = (void *)regs[REG_RIP];

	if (is_valid_addr((void *)(regs[REG_RIP] - 42), 64))
		dump_code(code_buf, rip);
	else
		memcpy(code_buf, "Invalid RIP!", sizeof("Invalid RIP!"));

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
		regs[REG_RIP], "",
		code_buf,
		regs[REG_RSP], regs[REG_EFL],
		regs[REG_RAX], regs[REG_RBX], regs[REG_RCX],
		regs[REG_RDX], regs[REG_RSI], regs[REG_RDI],
		regs[REG_RBP], regs[REG_R8], regs[REG_R9],
		regs[REG_R10], regs[REG_R11], regs[REG_R12],
		regs[REG_R13], regs[REG_R14], regs[REG_R15],
			/* cs */ cgfs[0],
			/* gs */ cgfs[1],
			/* fs */ cgfs[2],
			/* ss */ cgfs[3],
		regs[REG_CR2]
	);
}


static int __emerg_handler(int sig, siginfo_t *si, ucontext_t *ctx)
{
	int is_recoverable;
	mcontext_t *mctx = &ctx->uc_mcontext;
	uintptr_t *regs = (uintptr_t *)&mctx->gregs;
	void *rip = (void *)regs[REG_RIP];

	is_recoverable = (sig == SIGSEGV) ? 0 : is_emerg_pattern(rip);

	pr_intr("  Signal: %d (SIG%s); is_recoverable = %d;\n",
		sig, sigabbrev_np(sig), is_recoverable);

	dump_register(regs);

	pr_intr("  RSP Dump:\n");
	VT_HEXDUMP((void *)regs[REG_RSP], 512);
	print_backtrace((uintptr_t)rip);
	close(validator_fd);
	validator_fd = -1;
	return is_recoverable;
}


static void _emerg_handler(int sig, siginfo_t *si, ucontext_t *ctx)
{
	int is_recoverable;
	emerg_callback_t pre_emerg_print_trace = __pre_emerg_print_trace;
	emerg_callback_t post_emerg_print_trace = __post_emerg_print_trace;

	pr_intr("==========================================================\n");
	if (pre_emerg_print_trace)
		pre_emerg_print_trace(sig, si, ctx);

	is_recoverable = __emerg_handler(sig, si, ctx);

	if (post_emerg_print_trace)
		post_emerg_print_trace(sig, si, ctx);
	pr_intr("==========================================================\n");

	if (!is_recoverable)
		abort();
}


static void emerg_handler(int sig, siginfo_t *si, void *arg)
{
	int old;
	ucontext_t *ctx = (ucontext_t *)arg;

	if (sig == SIGSEGV && !(emerg_init_bits & EMERG_INIT_SIGSEGV))
		return;

	if (sig == SIGFPE && !(emerg_init_bits & EMERG_INIT_SIGFPE))
		return;

retry:
	old = lock_cmpxchg(&handler_lock, -1, 1);
	if (old != -1) {
		do {
			cpu_relax();
		} while (atomic_read(&handler_lock) != -1);
		goto retry;
	}
	_emerg_handler(sig, si, ctx);
	atomic_set(&handler_lock, -1);
}


int emerg_init_handler(unsigned bits)
{
	struct sigaction act;

	if (bits & ~EMERG_INIT_ALL)
		return -EINVAL;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = &emerg_handler;
	act.sa_flags = SA_SIGINFO;

	if (bits & (EMERG_INIT_BUG | EMERG_INIT_WARN | EMERG_INIT_SIGILL))
		sigaction(SIGILL, &act, NULL);

	if (bits & EMERG_INIT_SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);

	if (bits & EMERG_INIT_SIGFPE)
		sigaction(SIGFPE, &act, NULL);

	emerg_init_bits = bits;
	return 0;
}
