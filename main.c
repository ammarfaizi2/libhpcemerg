
#include <stdio.h>
#include <emerg/emerg.h>


void func_b(void)
{
	__asm__ volatile(
		"xorq\t%%rsp, %%rsp\n\t"
		"addq\t$100, %%rsp\n\t"
		"movq\t(%%rsp), %%rax\n\t"
		:
		:
		: "memory"
	);
	// for (size_t i = 0; i < 100; i++)
	// 	BUG_ON(1);
}


void func_a(void)
{
	func_b();
}

__attribute__((noinline))
void func_ss(int p)
{
	if (p > 1) {
		func_ss(p - 1);
	} else {
		func_a();
	}
}

int main(void)
{
	int ret = emerg_init_handler(EMERG_INIT_ALL);
	printf("ret = %d\n", ret);
	printf("Hello World!\n");
	func_ss(5);
	return 0;
}
