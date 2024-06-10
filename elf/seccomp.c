#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/stat.h>

void handler(int sig, siginfo_t *info, void *arg) {
  write(1, "Hello World\n", 13);
#ifdef __x86_64_
  uint64_t addr = ((uint64_t)info->si_call_addr) - 2;
  uint64_t maddr = (uint64_t)addr - (addr % 4096);
  mprotect((void *)maddr, 4096, PROT_WRITE | PROT_READ | PROT_EXEC);
  unsigned char *ptr = (void *)addr;
  *(ptr) = 0x90;
  *(ptr + 1) = 0xCC;
  mprotect((void *)maddr, 4096, PROT_READ | PROT_EXEC);
  void (*ret)() = ptr + 2;
  ret();
#endif
  syscall(SYS_exit, 0);
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int install_filter(unsigned int t_arch) {
  unsigned int upper_nr_limit = 0xffffffff;

  struct sock_filter filter[] = {
      /* [0] Load architecture from 'seccomp_data' buffer into
             accumulator. */
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),

      /* [1] Jump forward 5 instructions if architecture does not
             match 't_arch'. */
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, t_arch, 0, 4),

      /* [2] Load system call number from 'seccomp_data' buffer into
             accumulator. */
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

      /* [4] Jump forward 1 instruction if system call number
             does not match 'syscall_nr'. */
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_write, 4, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_mprotect, 3, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_rt_sigreturn, 2, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_exit, 1, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_read, 0, 1),
 

      /* [5] Matching architecture and system call: don't execute
         the system call, and return 'f_errno' in 'errno'. */
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

      /* [6] Destination of system call number mismatch: allow other
             system calls. */
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),

      /* [7] Destination of architecture mismatch: kill process. */
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
  };

  struct sock_fprog prog = {
      .len = ARRAY_SIZE(filter),
      .filter = filter,
  };

  if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog)) {
    perror("seccomp");
    return 1;
  }

  return 0;
}

int main() {
  struct sigaction sa;
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGSYS, &sa, NULL);
  prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
#ifdef __aarch64__ 
  install_filter(AUDIT_ARCH_AARCH64);
#else
  install_filter(AUDIT_ARCH_x86_64);
#endif
  kill(9, getpid());
  syscall(SYS_exit, 0);
  return 0;
}
