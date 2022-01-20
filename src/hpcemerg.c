// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdatomic.h>

static _Atomic(bool) stop_pool = false;
static _Atomic(bool) release_bug = false;
static _Atomic(bool) pool_is_online = false;
static struct hpcemerg_ctx *g_ctx = NULL;

#define __HPCEMERG_INTERNAL
#include "hpcemerg.h"

static void internal_hpcemerg_handler(int sig, siginfo_t *si, void *arg)
{
	struct hpcemerg_sig_ctx sig_ctx = {
		.sig		= sig,
		.si		= si,
		.ctx		= arg,
		.trap_data	= NULL
	};

	__arch_handle_trap_data(g_ctx, &sig_ctx);
	if (g_ctx->param.user_func)
		g_ctx->param.user_func(&sig_ctx);

	if (sig_ctx.trap_data) {
		if (sig_ctx.trap_data->type == HPCEMERG_TRAP_BUG) {
			while (!atomic_load(&release_bug))
				usleep(100000);
		}
	}
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

	atomic_store_explicit(&pool_is_online, true, memory_order_release);

	while (!atomic_load(&stop_pool))
		usleep(100000);

	atomic_store_explicit(&pool_is_online, false, memory_order_release);
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

void hpcemerg_set_release_bug(bool b)
{
	atomic_store_explicit(&release_bug, b, memory_order_release);
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
	g_ctx = ctx;
	return 0;

out_unlock:
	munlock(ctx, sizeof(*ctx));
out_unmap:
	munmap(ctx, sizeof(*ctx));
	return ret;
}

void hpcemerg_destroy(struct hpcemerg_ctx *ctx)
{
	atomic_store(&stop_pool, true);
	munlock(ctx, sizeof(*ctx));
	munmap(ctx, sizeof(*ctx));
	while (atomic_load(&pool_is_online))
		usleep(10000);
}

size_t hpcemerg_register_dump(char *buffer, size_t maxlen,
			      struct hpcemerg_sig_ctx *sig_ctx)
{
	return __arch_hpcemerg_register_dump(buffer, maxlen, sig_ctx);
}
