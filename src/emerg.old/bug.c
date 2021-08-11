




static void __print_warn(const char *file, int line, const char *func)
{
	pr_intr("  WARNING: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


static void __print_bug(const char *file, int line, const char *func)
{
	pr_intr("  BUG: PID: %d at %s:%d %s\n", getpid(), file, line, func);
}


static void register_dump_print(unsigned long *gregs)
{
	unsigned short cgfs[4]; /* unsigned short cs, gs, fs, ss.  */
	memcpy(cgfs, &gregs[REG_CSGSFS], sizeof(cgfs));
	pr_intr(
		"  RIP: %016lx\n"
		"  Code: %s\n"
		"  RSP: %016lx EFLAGS: %08lx\n"
		"  RAX: %016lx RBX: %016lx RCX: %016lx\n"
		"  RDX: %016lx RSI: %016lx RDI: %016lx\n"
		"  RBP: %016lx R08: %016lx R09: %016lx\n"
		"  R10: %016lx R11: %016lx R12: %016lx\n"
		"  R13: %016lx R14: %016lx R15: %016lx\n"
		"  CS: %04x GS: %04x FS: %04x SS: %04x\n"
		"  CR2: %016lx\n\n",
		gregs[REG_RIP],
		code_buf,
		gregs[REG_RSP], gregs[REG_EFL],
		gregs[REG_RAX], gregs[REG_RBX], gregs[REG_RCX],
		gregs[REG_RDX], gregs[REG_RSI], gregs[REG_RDI],
		gregs[REG_RBP], gregs[REG_R8], gregs[REG_R9],
		gregs[REG_R10], gregs[REG_R11], gregs[REG_R12],
		gregs[REG_R13], gregs[REG_R14], gregs[REG_R15],
			/* cs */ cgfs[0],
			/* gs */ cgfs[1],
			/* fs */ cgfs[2],
			/* ss */ cgfs[3],
		gregs[REG_CR2]
	);
}


void emerg_print_trace(int sig, siginfo_t *si, void *arg)
{
	ucontext_t *ctx = (ucontext_t *)arg;
	mcontext_t *mctx = &ctx->uc_mcontext;
	unsigned long *gregs = (unsigned long *)&mctx->gregs;
}


void emerg_print_trace_init(void)
{
	struct sigaction action;
	action.sa_sigaction = &emerg_print_trace;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
}
