
#include <stdio.h>
#include <emerg/emerg.h>


void func_b(void)
{
	WARN_ON(1);
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
