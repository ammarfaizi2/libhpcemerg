
#include <stdio.h>
#include <libhpcemerg.h>

extern void fentry_target(void);

#define noinline __attribute__((__noinline__))

noinline void func_a(void)
{
	puts(__FUNCTION__);
}

noinline void func_b(void)
{
	puts(__FUNCTION__);	
}

noinline void func_c(void)
{
	puts(__FUNCTION__);
}

noinline void func_d(double test)
{
	printf("test = %f\n", test);
}

int main(void)
{
	double test = 1000;
	__fentry_set_target_func(fentry_target);
	__fentry_enable();
	func_a();
	func_b();
	func_c();
	func_d(test);
	return 0;
}
