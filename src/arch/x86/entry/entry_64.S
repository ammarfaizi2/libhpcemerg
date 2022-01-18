// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C)  2022 Ammar Faizi <ammarfaizi2@gmail.com>
 */

.section .data
enabled:
	.byte 0
target_func:
	.quad 0

.section .text
.global hpcemerg_fentry_enable
.global hpcemerg_fentry_disable
.global hpcemerg_fentry_set_target_func
.global __fentry__

hpcemerg_fentry_enable:
	movb	$1, enabled(%rip)
	retq

hpcemerg_fentry_disable:
	movb	$0, enabled(%rip)
	retq

hpcemerg_fentry_set_target_func:
	movq	%rdi, target_func(%rip)
	retq

__fentry__:
	cmpb	$0, enabled(%rip)
	jne	1f
	retq
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	callq	*target_func(%rip)
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	retq
