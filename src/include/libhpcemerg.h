// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */
#ifndef LIBHPCEMERG_H__
#define LIBHPCEMERG_H__

extern void __fentry_enable(void);
extern void __fentry_disable(void);
extern void __fentry_set_target_func(void (*target)(void));

#endif /* #ifndef LIBHPCEMERG_H__ */
