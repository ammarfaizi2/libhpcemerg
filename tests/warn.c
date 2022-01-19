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

static struct hpcemerg_trap_data g_td;
static struct hpcemerg_ctx *hpcemerg_ctx = NULL;

static void handler(struct hpcemerg_sig_ctx *sig_ctx)
{
	if (sig_ctx->sig != SIGILL) {
		/*
		 * WARN*() generates SIGILL.
		 */
		fprintf(stderr, "sig != SIGILL\n");
		exit(1);
	}

	if (sig_ctx->trap_data == NULL) {
		/*
		 * WARN*() must set trap_data.
		 */
		fprintf(stderr, "sig_ctx->trap_data == NULL\n");
		exit(1);
	}

	if (sig_ctx->trap_data->type != HPCEMERG_TRAP_WARN) {
		fprintf(stderr,
			"sig_ctx->trap_data.type != HPCEMERG_TRAP_WARN)\n");
		exit(1);
	}

	/*
	 * Copy the trap_data to a global variable,
	 * we will inspect the value from ASSERT_WARN().
	 */
	memcpy(&g_td, sig_ctx->trap_data, sizeof(g_td));
}

#define ASSERT_WARN(FILE, LINE, FUNC)					\
do {									\
	uint32_t _line = (LINE);					\
	const char *_file = (FILE);					\
	const char *_func = (FUNC);					\
	if (_line != g_td.line) {					\
		fprintf(stderr, "LINE != g_td.line (%u != %u)\n", _line,\
			g_td.line);					\
		return 1;						\
	}								\
									\
	if (strcmp(_file, g_td.file)) {					\
		fprintf(stderr, "FILE != g_td.file (%s != %s)\n", _file,\
			g_td.file);					\
		return 1;						\
	}								\
									\
	if (strcmp(_func, g_td.func)) {					\
		fprintf(stderr, "FUNC != g_td.func (%s != %s)\n", _func,\
			g_td.func);					\
		return 1;						\
	}								\
} while (0)

static int simple_warn(void)
{
	WARN_ON(1);
	ASSERT_WARN(__FILE__, __LINE__ - 1, __func__);
	WARN();
	ASSERT_WARN(__FILE__, __LINE__ - 1, __func__);
	return 0;
}

static int warn_with_loop(void)
{
	int i;
	for (i = 0; i < 10; i++) {
		/*
		 * WARN_ONCE() should only be called
		 * once in this "for loop".
		 */
		WARN_ONCE();
		if (i == 0)
			ASSERT_WARN(__FILE__, __LINE__ - 2, __func__);
		else
			ASSERT_WARN(__FILE__, 999, __func__);
		g_td.line = 999;
	}
	return 0;
}

int main(void)
{
	int ret;
	struct hpcemerg_init_param p = {
		.user_func	= &handler,
		.handle_bits	= HPCEMERG_HANDLE_WARN
	};


	ret = hpcemerg_init(&p, &hpcemerg_ctx);
	if (ret) {
		errno = -ret;
		perror("hpcemerg_init");
		return -ret;
	}

	ret = simple_warn();
	if (ret)
		return ret;

	ret = warn_with_loop();
	return ret;
}
