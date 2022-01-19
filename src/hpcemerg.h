// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef LIBHPCEMERG_H__
#define LIBHPCEMERG_H__

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <ucontext.h>
#include <execinfo.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __always_inline
#define __always_inline __attribute__((__always_inline__))
#endif

#define HPCEMERG_INTR_STKSZ (1024 * 1024)

#include <dlfcn.h>

struct hpcemerg_sig_ctx {
	int				sig;
	siginfo_t			*si;
	ucontext_t			*ctx;
	struct hpcemerg_trap_data	*trap_data;
};

struct hpcemerg_init_param {
	void		(*user_func)(struct hpcemerg_sig_ctx *sig_ctx);
	uint32_t	handle_bits;
	uint32_t	resv[3];
};

struct hpcemerg_ctx {
	uint8_t				stack[HPCEMERG_INTR_STKSZ];
	struct hpcemerg_init_param	param;
	pthread_t			thread;
};

extern void hpcemerg_fentry_enable(void);
extern void hpcemerg_fentry_disable(void);
extern void hpcemerg_fentry_set_target_func(void (*target)(void));

extern int hpcemerg_init(struct hpcemerg_init_param *p,
			 struct hpcemerg_ctx **ctx_p);
extern void hpcemerg_destroy(struct hpcemerg_ctx *p);
extern void hpcemerg_set_release_bug(bool b);

#define HPCEMERG_HANDLE_BUG	(1u << 0u)
#define HPCEMERG_HANDLE_WARN	(1u << 1u)
#define HPCEMERG_HANDLE_SIGSEGV	(1u << 2u)
#define HPCEMERG_HANDLE_SIGILL	(1u << 3u)
#define HPCEMERG_HANDLE_SIGFPE	(1u << 4u)
#define HPCEMERG_HANDLE_ALL (		\
	HPCEMERG_HANDLE_BUG	|	\
	HPCEMERG_HANDLE_WARN	|	\
	HPCEMERG_HANDLE_SIGSEGV	|	\
	HPCEMERG_HANDLE_SIGILL	|	\
	HPCEMERG_HANDLE_SIGILL		\
)

#define HPCEMERG_TRAP_BUG	(1u << 0u)
#define HPCEMERG_TRAP_WARN	(1u << 1u)

#define IN_HPCEMERG_H__
#if defined(__x86_64__)
	#include "arch/x86/hpcemerg.h"
#endif

#endif /* #ifndef LIBHPCEMERG_H__ */
