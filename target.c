
#include <libhpcemerg.h>

#define CALL_FPU_FUNC(FUNC_NAME)		\
do {						\
__asm__ volatile(				\
	"pushq	%%rbp\n\t"			\
	"movq	%%rsp, %%rbp\n\t"		\
	"andq	$-16, %%rsp\n\t"		\
	"subq	$(16 * 8), %%rsp\n\t"		\
	"movdqa	%%xmm0, 0*16(%%rsp)\n\t"	\
	"movdqa	%%xmm1, 1*16(%%rsp)\n\t"	\
	"movdqa	%%xmm2, 2*16(%%rsp)\n\t"	\
	"movdqa	%%xmm3, 3*16(%%rsp)\n\t"	\
	"movdqa	%%xmm4, 4*16(%%rsp)\n\t"	\
	"movdqa	%%xmm5, 5*16(%%rsp)\n\t"	\
	"movdqa	%%xmm6, 6*16(%%rsp)\n\t"	\
	"movdqa	%%xmm7, 7*16(%%rsp)\n\t"	\
	"callq	" # FUNC_NAME "\n\t"		\
	"movdqa	7*16(%%rsp), %%xmm7\n\t"	\
	"movdqa	6*16(%%rsp), %%xmm6\n\t"	\
	"movdqa	5*16(%%rsp), %%xmm5\n\t"	\
	"movdqa	4*16(%%rsp), %%xmm4\n\t"	\
	"movdqa	3*16(%%rsp), %%xmm3\n\t"	\
	"movdqa	2*16(%%rsp), %%xmm2\n\t"	\
	"movdqa	1*16(%%rsp), %%xmm1\n\t"	\
	"movdqa	0*16(%%rsp), %%xmm0\n\t"	\
	"movq	%%rbp, %%rsp\n\t"		\
	"popq	%%rbp\n\t"			\
	:					\
	:					\
	: "rax", "rdi", "rsi", "rdx", "rcx",	\
	  "r8", "r9", "r10", "r11", "memory"	\
);						\
} while (0)


#include <stdio.h>

#define CALL_FPU_FUNC_R(FUNC_NAME, ARG)		\
do {						\
	void *__rdi_out;			\
__asm__ volatile(				\
	"pushq	%%rbp\n\t"			\
	"movq	%%rsp, %%rbp\n\t"		\
	"andq	$-16, %%rsp\n\t"		\
	"subq	$(16 * 8), %%rsp\n\t"		\
	"movdqa	%%xmm0, 0*16(%%rsp)\n\t"	\
	"movdqa	%%xmm1, 1*16(%%rsp)\n\t"	\
	"movdqa	%%xmm2, 2*16(%%rsp)\n\t"	\
	"movdqa	%%xmm3, 3*16(%%rsp)\n\t"	\
	"movdqa	%%xmm4, 4*16(%%rsp)\n\t"	\
	"movdqa	%%xmm5, 5*16(%%rsp)\n\t"	\
	"movdqa	%%xmm6, 6*16(%%rsp)\n\t"	\
	"movdqa	%%xmm7, 7*16(%%rsp)\n\t"	\
	"callq	" # FUNC_NAME "\n\t"		\
	"movdqa	7*16(%%rsp), %%xmm7\n\t"	\
	"movdqa	6*16(%%rsp), %%xmm6\n\t"	\
	"movdqa	5*16(%%rsp), %%xmm5\n\t"	\
	"movdqa	4*16(%%rsp), %%xmm4\n\t"	\
	"movdqa	3*16(%%rsp), %%xmm3\n\t"	\
	"movdqa	2*16(%%rsp), %%xmm2\n\t"	\
	"movdqa	1*16(%%rsp), %%xmm1\n\t"	\
	"movdqa	0*16(%%rsp), %%xmm0\n\t"	\
	"movq	%%rbp, %%rsp\n\t"		\
	"popq	%%rbp\n\t"			\
	: "=D"(__rdi_out)			\
	: "D"(ARG)				\
	: "rax", "rsi", "rdx", "rcx", "r8",	\
	  "r9", "r10", "r11", "memory"		\
);						\
	(void) __rdi_out;			\
} while (0)


void do_fpu_operation(void *arg)
{
	double test = 9999;
	printf("test = %f\n", test);
	printf("string = %s\n", (const char *)arg);
}

void fentry_target(void)
{
	const char arg[] = "Hello World!";
	puts("entry...");
	CALL_FPU_FUNC_R(do_fpu_operation, arg);
}
