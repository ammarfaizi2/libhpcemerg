// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 *
 * Simple test to handle SIGILL for libhpcemerg.
 */
#include <hpcemerg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static struct hpcemerg_ctx *hpcemerg_ctx = NULL;

static void handler(struct hpcemerg_sig_ctx *sig_ctx)
{
	int ret = 1;

	if (sig_ctx->sig != SIGILL) {
		fprintf(stderr, "sig != SIGILL\n");
		goto out;
	}

	ret = 0;
out:
	exit(ret);
}

int main(void)
{
	int ret;
	struct hpcemerg_init_param p = {
		.user_func	= &handler,
		.handle_bits	= HPCEMERG_HANDLE_SIGILL
	};


	ret = hpcemerg_init(&p, &hpcemerg_ctx);
	if (ret) {
		errno = -ret;
		perror("hpcemerg_init");
		return -ret;
	}

	__asm__ volatile("ud2":::"memory");

	// Shouldn't be reached, we exit from the handler.
	return 1;
}
