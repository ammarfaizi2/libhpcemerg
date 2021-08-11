// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef EMERG__SRC__BUG_H
#define EMERG__SRC__BUG_H

#ifndef ____stringify
#define ____stringify(X) #X
#endif

#ifndef __stringify
#define __stringify(X) ____stringify(X)
#endif

#ifndef unlikeky
#define unlikeky(COND) __builtin_expect(!!(COND), 0)
#endif

#ifndef likeky
#define likeky(COND) __builtin_expect(!!(COND), 1)
#endif

#define EMERG_TYPE_WARN		(0)
#define EMERG_TYPE_BUG		(1)

struct emerg_entry____ {
	const char		*file;
	const char		*func;
	unsigned int		line;
	unsigned short		taint;
	unsigned char		emerg_type;
};


/*
 * Despite that some emulators terminate on UD2, we use it for WARN().
 */
#define ASM_UD2		".byte 0x0f, 0x0b"
#define INSN_UD2	0x0b0f
#define LEN_UD2		2


#define ASM_EMERG__(INS, TAINT, TYPE)					\
do {									\
	static const struct emerg_entry____ ____emerg_entry = {		\
		__FILE__,						\
		__func__,						\
		(unsigned int)__LINE__,					\
		TAINT,							\
		(TYPE)							\
	};								\
									\
	__asm__ inline volatile(					\
		"# BUG!!!\n\t"						\
		INS "\n\t"						\
		".quad	%c0"						\
		:							\
		: "i"(&____emerg_entry)					\
		:							\
	);								\
} while (0)


#define BUG(TAINT)					\
do {							\
	ASM_EMERG__(ASM_UD2, TAINT, EMERG_TYPE_BUG);	\
} while (0)


#define BUG_ON(COND) ({				\
	int ____condition = !!(COND);		\
	if (unlikeky(____condition))		\
		BUG(0);				\
	unlikeky(____condition);		\
})


/* TODO: Safely print in interrupt handler. */
#define pr_intr printf

extern void __print_bug(const char *file, int line, const char *func);
extern void __print_warn(const char *file, int line, const char *func);

#ifdef IN_PRINT_TRACE_C
	extern void emerg_print_trace(int sig, siginfo_t *si, void *arg);
#else
	extern void emerg_print_trace(int sig, void *si, void *arg);
#endif

extern void emerg_print_trace_init(void);

#endif /* #ifndef EMERG__SRC__BUG_H */
