#include <seccomp.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <syscall.h>

int seccomp_start() {
	int rc = -1;
        scmp_filter_ctx ctx;
        ctx = seccomp_init(SCMP_ACT_TRAP);
        if (ctx == NULL)
             goto out;
        rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
	rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
        rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
	rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
	rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
        rc = seccomp_load(ctx);
        if (rc < 0)
           goto out;
	return 0;

out:
        seccomp_release(ctx);
        return -rc;
}

void handler(int sig, siginfo_t* info, void* arg) {
	write(1, "Hello World\n", 13);
	uint64_t addr = (uint64_t)info->si_call_addr;
	uint64_t maddr = (uint64_t) addr - (addr % 4096);
	mprotect((void*) maddr, 4096, PROT_WRITE | PROT_READ);
	unsigned char* ptr = (void*)addr;
	*(ptr) = 0x90;
	*(ptr + 1) = 0xCC;
	mprotect((void*)maddr, 4096, PROT_READ | PROT_EXEC);
}


int main() {
	struct sigaction sa;
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSYS, &sa, NULL);
	seccomp_start();
	kill(1, getpid());
	return 0;
}
