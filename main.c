
#include <stdio.h>
#include <emerg/emerg.h>


void func_b(void)
{
	WARN_ON(1);
	WARN_ON(1);
	WARN_ON(1);
	BUG_ON(1);
	BUG_ON(1);
}


void func_a(void)
{
	func_b();
}

__attribute__((noinline))
void func_ss(int p)
{
	if (p > 1) {
		BUG_ON(1);
		__asm__ volatile(
			"decl	%0"
			: "+r"(p)
			:
			: "memory"
		);
		printf("%d\n", p);
		func_ss(p);
	} else {
		func_a();
	}
}

int main(void)
{
	emerg_print_trace_init();
	printf("Hello World!\n");
	func_ss(100);
	return 0;
}
