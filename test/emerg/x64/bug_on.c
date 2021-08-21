// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */


__attribute__((noinline))
void func_c(void)
{
	func_d();
	__asm__ volatile("":::"memory");
}


__attribute__((noinline))
void func_b(void)
{
	func_c();
	__asm__ volatile("":::"memory");
}


__attribute__((noinline))
void func_a(void)
{
	func_b();
	__asm__ volatile("":::"memory");
}


int main(void)
{
	func_a();
}
