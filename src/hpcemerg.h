// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef LIBHPCEMERG_H__
#define LIBHPCEMERG_H__

extern void hpcemerg_enable(void);
extern void hpcemerg_disable(void);
extern void hpcemerg_set_target_func(void (*target)(void));

#endif /* #ifndef LIBHPCEMERG_H__ */
