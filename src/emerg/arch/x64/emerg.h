// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef IN_THE_MAIN_EMERG_H
#error "This header must only be included from emerg/emerg.h"
#endif

#ifndef EMERG__SRC__X64__EMERG_H
#define EMERG__SRC__X64__EMERG_H


/*
 * Despite that some emulators terminate on UD2, we use it.
 */
#define ASM_UD2		".byte 0x0f, 0x0b"
#define INSN_UD2	0x0b0f
#define LEN_UD2		2


#define ASM_EMERG__(INS, TAINT, TYPE)					\
do {									\
	static const struct emerg_entry ____emerg_entry = {		\
		__FILE__,						\
		__func__,						\
		(unsigned int)__LINE__,					\
		TAINT,							\
		(TYPE)							\
	};								\
									\
	__asm__ inline volatile(					\
		"# ASM_EMERG__\n\t"					\
		INS "\n\t"						\
		"leaq	%0, %%rax"					\
		:							\
		: "m"(____emerg_entry)					\
		:							\
	);								\
} while (0)


#define WARN() ({					\
	ASM_EMERG__(ASM_UD2, 0, EMERG_TYPE_WARN);	\
	(1);						\
})


#define WARN_ONCE() ({						\
	static bool __is_warned = false;			\
	if (!__is_warned) {					\
		__is_warned = true;				\
		ASM_EMERG__(ASM_UD2, 0, EMERG_TYPE_WARN);	\
	}							\
	(1);							\
})


#define WARN_ON(COND)				\
do {						\
	bool __cond = (COND);			\
	if (__cond)				\
		WARN();				\
	(__cond);				\
} while (0)


#define WARN_ON_ONCE(COND)			\
do {						\
	bool __cond = (COND);			\
	static bool __is_warned = false;	\
	if (__cond && !__is_warned) {		\
		__is_warned = true;		\
		WARN();				\
	}					\
	(__cond);				\
} while (0)

#endif /* #ifndef EMERG__SRC__X64__EMERG_H */
