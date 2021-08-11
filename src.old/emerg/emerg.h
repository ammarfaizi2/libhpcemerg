// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef EMERG__SRC__EMERG_H
#define EMERG__SRC__EMERG_H

#include <stdbool.h>
#include <stdio.h>
#define pr_intr printf

#define __USE_GNU
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>

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

#ifndef pr_intr
/* TODO: Safely print in interrupt handler. */
#define pr_intr printf
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

#define IN_THE_MAIN_EMERG_H
#include "x64/emerg.h"
#undef IN_THE_MAIN_EMERG_H


#define BUG(TAINT)					\
do {							\
	ASM_EMERG__(ASM_UD2, TAINT, EMERG_TYPE_BUG);	\
} while (0)


#define BUG_ON(COND) ({				\
	int ____condition = !!(COND);		\
	if (unlikeky(____condition))		\
		BUG(1);				\
	(unlikeky(____condition));		\
})


#define BUG_TAINT_ON(COND, TAINT) ({		\
	int ____condition = !!(COND);		\
	if (unlikeky(____condition))		\
		BUG(TAINT);			\
	(unlikeky(____condition));		\
})


#define WARN(TAINT)					\
do {							\
	ASM_EMERG__(ASM_UD2, TAINT, EMERG_TYPE_WARN);	\
} while (0)


#define WARN_ON(COND) ({			\
	int ____condition = !!(COND);		\
	if (unlikeky(____condition))		\
		WARN(1);			\
	(unlikeky(____condition));		\
})


#define WARN_TAINT_ON(COND, TAINT) ({		\
	int ____condition = !!(COND);		\
	if (unlikeky(____condition))		\
		WARN(TAINT);			\
	(unlikeky(____condition));		\
})


typedef bool (*emerg_callback_t)(int sig, siginfo_t *si, ucontext_t *arg);

extern volatile short __emerg_taint;
extern volatile emerg_callback_t __pre_emerg_print_trace;
extern volatile emerg_callback_t __post_emerg_print_trace;
extern void emerg_print_trace_init(void);

#endif /* #ifndef EMERG__SRC__EMERG_H */
