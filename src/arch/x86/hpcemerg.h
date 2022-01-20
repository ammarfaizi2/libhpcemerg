// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef IN_HPCEMERG_H__
	#error "This header should only be included from the main hpcemerg.h"
#endif

#ifndef ARCH__X86__HPCEMERG_H__
#define ARCH__X86__HPCEMERG_H__

/*
 * Despite that some emulators terminate on UD2, we use it.
 */
#define ASM_UD2		".byte 0x0f, 0x0b"
#define INSN_UD2	0x0b0f
#define LEN_UD2		2


struct hpcemerg_trap_data {
	const char		*file;
	const char		*func;
	uint32_t		line;
	uint8_t			type;
	uint8_t			resv[3];
};


#define HPCEMERG_SET_TRAP(INSN, TYPE)					\
do {									\
	static const struct hpcemerg_trap_data __trap_data = {		\
		.file = __FILE__,					\
		.func = __func__,					\
		.line = (uint32_t) __LINE__,				\
		.type = (TYPE)						\
	};								\
	__asm__ volatile(						\
		"# HPCEMERG_SET_TRAP\n\t"				\
		INSN "\n\t"						\
		"leaq	%[__trap_data], %%rax"				\
		:							\
		: [__trap_data]"m"(__trap_data)				\
		: "memory"						\
	);								\
} while (0)


#define WARN()						\
do {							\
	HPCEMERG_SET_TRAP(ASM_UD2, HPCEMERG_TRAP_WARN);	\
} while (0)						\


#define WARN_ONCE()					\
do {							\
	static bool __is_warned = false;		\
	if (__builtin_expect(!__is_warned, 0)) {	\
		__is_warned = true;			\
		WARN();					\
	}						\
} while (0)

#define WARN_ON(COND) ({			\
	bool __cond = (COND);			\
	if (__builtin_expect(__cond, 0))	\
		WARN();				\
	(__cond);				\
})

#define WARN_ON_ONCE(COND) ({			\
	bool __cond = (COND);			\
	if (__builtin_expect(__cond, 0))	\
		WARN_ONCE();			\
	(__cond);				\
})

#define BUG()						\
do {							\
	HPCEMERG_SET_TRAP(ASM_UD2, HPCEMERG_TRAP_BUG);	\
} while (0)						\

#define BUG_ON(COND) ({				\
	bool __cond = (COND);			\
	if (__builtin_expect(__cond, 0))	\
		BUG();				\
	(__cond);				\
})


#ifdef __HPCEMERG_INTERNAL

#include <stdio.h>
#include <string.h>

__always_inline
static bool is_hpcemerg_trap_pattern(void *rip)
{
	/*
	 * "\x0f\x0b"		ud2
	 * "\x48\x8d\x05"	lea rax, [rip + rel32]
	 */
	return !memcmp("\x0f\x0b\x48\x8d\x05", rip, 5);
}

__always_inline
static struct hpcemerg_trap_data *get_trap_data(void *rip)
{
	int32_t rel32;
	void *lea_pos = (void *) ((uintptr_t) rip + 5);

	memcpy(&rel32, lea_pos, sizeof(rel32));
	if (rel32 > 0) {
		/*
		 * Positive relative offset.
		 */
		rip = (void *) ((uintptr_t) rip + 5 + 4 + (uintptr_t) rel32);
	} else {
		/*
		 * Negative relative offset.
		 */
		rel32 *= -1;
		rip = (void *) ((uintptr_t) rip + 5 + 4);
		rip = (void *) ((uintptr_t) rip - (uintptr_t) rel32);
	}
	return rip;
}

__always_inline
static void handle_trap(struct hpcemerg_ctx *ctx,
			struct hpcemerg_sig_ctx *sig_ctx)
{
	unsigned type;
	uint32_t hb = ctx->param.handle_bits;
	ucontext_t *uctx = sig_ctx->ctx;
	mcontext_t *mctx = &uctx->uc_mcontext;
	uintptr_t *regs = (uintptr_t *) &mctx->gregs;
	void *rip = (void *) regs[REG_RIP];

	if (!is_hpcemerg_trap_pattern(rip))
		raise(sig_ctx->sig);

	sig_ctx->trap_data = get_trap_data(rip);

	/* Skip the ud2 and lea. */
	regs[REG_RIP] += 5 + 4;

	type = sig_ctx->trap_data->type;
	if (type == HPCEMERG_TRAP_WARN && !(hb & HPCEMERG_HANDLE_WARN))
		raise(sig_ctx->sig);
	if (type == HPCEMERG_TRAP_BUG && !(hb & HPCEMERG_HANDLE_BUG))
		raise(sig_ctx->sig);
}

__always_inline
static void __arch_handle_trap_data(struct hpcemerg_ctx *ctx,
				    struct hpcemerg_sig_ctx *sig_ctx)
{
	switch (sig_ctx->sig) {
	case SIGILL:
		if (ctx->param.handle_bits & (HPCEMERG_HANDLE_WARN |
					      HPCEMERG_HANDLE_BUG))
			handle_trap(ctx, sig_ctx);
		break;
	}
}

#endif /* #ifdef __HPCEMERG_INTERNAL */

#endif /* #ifndef ARCH__X86__HPCEMERG_H__ */
