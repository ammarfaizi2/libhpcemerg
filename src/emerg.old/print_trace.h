// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef EMERG__SRC__PRINT_TRACE_H
#define EMERG__SRC__PRINT_TRACE_H

#include <stdbool.h>

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


#define EMERG_PR_UNKNOWN 0x0

#define SET_FILE_PTR					\
	".rodata "

#define EMERG_PR_WARN_ON 0x1
#define WARN_ON_OPCODE "\x0f\x0b\xff\xff\xff\x01"
#define WARN_ON_OPCODE_ASM				\
do {							\
	__asm__ volatile(				\
		".byte 0x0f, 0x0b\n\t" /* ud2 */	\
		".byte 0xff,0xff,0xff,0x01"		\
	);						\
} while (0)

#define WARN_ON(COND)						\
({								\
	bool ____cond = (COND);					\
	if (____cond) {						\
		WARN_ON_OPCODE_ASM;				\
	}							\
	(____cond);						\
})



#define EMERG_PR_BUG_ON 0x2
#define BUG_ON_OPCODE "\x0f\x0b\xff\xff\xff\x02"
#define BUG_ON_OPCODE_ASM				\
do {							\
	__asm__ volatile(				\
		".byte 0x0f, 0x0b\n\t" /* ud2 */	\
		".byte 0xff,0xff,0xff,0x02"		\
	);						\
} while (0)

#define BUG_ON(COND)						\
({								\
	bool ____cond = (COND);					\
	if (____cond) {						\
		BUG_ON_OPCODE_ASM;				\
	}							\
	(____cond);						\
})

#endif /* #ifndef EMERG__SRC__PRINT_TRACE_H */
