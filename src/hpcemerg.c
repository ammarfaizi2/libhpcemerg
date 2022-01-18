// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "hpcemerg.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>

static void internal_hpcemerg_handler(int sig, siginfo_t *si, void *arg)
{
	(void) sig;
	(void) si;
	(void) arg;
}

static int install_signal_handler(struct hpcemerg_ctx *ctx)
{
	struct sigaction act;
	uint32_t hbits = ctx->param.handle_bits;

	if (hbits & HPCEMERG_HANDLE_ALL) {
		stack_t st;
		st.ss_sp = ctx->stack;
		st.ss_flags = 0;
		st.ss_size = sizeof(ctx->stack);
		if (sigaltstack(&st, NULL))
			return -errno;
	}

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = &internal_hpcemerg_handler;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;

	if (hbits & (HPCEMERG_HANDLE_BUG | HPCEMERG_HANDLE_WARN |
		     HPCEMERG_HANDLE_SIGILL))
		sigaction(SIGILL, &act, NULL);

	if (hbits & HPCEMERG_HANDLE_SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);

	if (hbits & HPCEMERG_HANDLE_SIGFPE)
		sigaction(SIGFPE, &act, NULL);

	return 0;
}

static int _hpcemerg_init(struct hpcemerg_ctx *ctx)
{
	int ret;
	struct hpcemerg_init_param *p = &ctx->param;

	if (p->handle_bits & ~HPCEMERG_HANDLE_ALL)
		return -EINVAL;

	ret = install_signal_handler(ctx);
	if (ret < 0)
		return ret;

	return 0;
}

static void *emergency_thread_pool(void *ctx_v)
{
	struct hpcemerg_ctx *ctx = ctx_v;

	(void) ctx;
	return NULL;
}

static int spawn_emergency_pool(struct hpcemerg_ctx *ctx)
{
	int ret;

	ret = pthread_create(&ctx->thread, NULL, emergency_thread_pool, ctx);
	if (ret)
		return ret;

	pthread_detach(ctx->thread);
	pthread_setname_np(ctx->thread, "hpcemerg-pool");
	return 0;
}

int hpcemerg_init(struct hpcemerg_init_param *p, struct hpcemerg_ctx **ctx_p)
{
	int ret;
	struct hpcemerg_ctx *ctx;

	ctx = mmap(NULL, sizeof(*ctx), PROT_READ | PROT_WRITE,
		   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ctx == MAP_FAILED)
		return -errno;

	if (mlock(ctx, sizeof(*ctx)) < 0) {
		ret = -errno;
		goto out_unmap;
	}

	ctx->param = *p;
	ret = _hpcemerg_init(ctx);
	if (ret < 0)
		goto out_unlock;

	ret = spawn_emergency_pool(ctx);
	if (ret < 0)
		goto out_unlock;

	*ctx_p = ctx;
	return 0;

out_unlock:
	munlock(ctx, sizeof(*ctx));
out_unmap:
	munmap(ctx, sizeof(*ctx));
	return ret;
}
