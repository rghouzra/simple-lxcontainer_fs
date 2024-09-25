#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define PS_STACK_SIZE (1024 * 1024) // ~ 1k Gb

int child_fn(void *arg) {
  printf("child pid inside namespace: %d\n", getpid());
  printf("child parent pid inside namespace: %d\n", getppid());
  if (unshare(CLONE_NEWNS) == -1) {
    perror("unshare: ");
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  char *p_stack = malloc(PS_STACK_SIZE);
  if (p_stack == NULL) {
    perror("malloc: ");
    exit(1);
  }
  pid_t pid =
      clone(child_fn, p_stack + PS_STACK_SIZE, CLONE_NEWPID | SIGCHLD, NULL);
  if (pid < 0) {
    perror("clone: ");
    exit(1);
  }
  printf("parent pid: %d\n", getpid());
  printf("Child's pid inside the parent namespace: %d\n", pid);
  if (waitpid(pid, NULL, 0) == -1) {
    perror("waitpid: ");
    exit(1);
  }
  free(p_stack);
}
