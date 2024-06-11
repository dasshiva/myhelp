#define _GNU_SOURCE
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/prctl.h>
#include <ucontext.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/stat.h>

void handler(int sig, siginfo_t *info, ucontext_t *arg) {
  write(1, "Hello World\n", 13);
#ifdef __x86_64__
  uint64_t addr = ((uint64_t)info->si_call_addr) - 2;
  uint64_t maddr = (uint64_t)addr - (addr % 4096);
  mprotect((void *)maddr, 4096, PROT_WRITE | PROT_READ | PROT_EXEC);
  unsigned char *ptr = (void *)addr;
  *(ptr) = 0x90;
  *(ptr + 1) = 0xCC;
  mprotect((void *)maddr, 4096, PROT_READ | PROT_EXEC);
  arg->uc_mcontext.gregs[REG_RAX] = 2;
  arg->uc_mcontext.gregs[REG_RIP] = ptr + 2;
#elif __aarch64__
  syscall(SYS_exit, 0);
 #endif
}

/* The below function install_filter() and the macro ARRAY_SIZE is a modified version 
 * of the source code available at https://man7.org/linux/man-pages/man2/seccomp.2.html 
 * in the "Examples" section */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int install_filter() {
  // To keep the filter small, we do not check the architecture because it is known at
  // compile time which arhitecture we are compiling for
  
  // To add new system calls that need to be allowed, add them at the top of the first
  // BPF_JUMP statement and with a number one more than the first statement's 'jt' field
  struct sock_filter filter[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_write, 4, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_mprotect, 3, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_rt_sigreturn, 2, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_exit, 1, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_read, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),
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
  install_filter();
  long n = syscall(SYS_mmap);
  syscall(SYS_exit, 0);
  return 0;
}
