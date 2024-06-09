#include <seccomp.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
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
	syscall(SYS_exit, 1);
}


int main() {
	struct sigaction sa;
	sa.sa_sigaction = handler;
	sigaction(SIGSYS, &sa, NULL);
	seccomp_start();
	kill(1, getpid());
	return 0;
}
