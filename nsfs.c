#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define PS_STACK_SIZE (1024 * 1024) // ~ 1k Gb

int child_fn(void *arg) {
  printf("child pid inside namespace: %d\n", getpid());
  printf("child parent pid inside namespace: %d\n", getppid());
  if((char **)arg != NULL) {
    char **argv = (char **)arg;
    execv(argv[0], argv);
    return 1;
  }
  if (unshare(CLONE_NEWNS) == -1) {
    perror("unshare: ");
    return 1;
  }
  // mount a tmpfs for our new root
  if(mount("none", "/tmp", "tmpfs", 0, "") == -1) {
    perror("mount: ");
    return 1;
  }
  // create a new directory for the new root
  if(mkdir("/tmp/bin", 0755) == -1 || mkdir("/tmp/lib", 0755) == -1 || mkdir("/tmp/lib64", 0755) == -1) {
    perror("mkdir: ");
    return 1;
  }
  // bind mount necessary dir from the host(bins)
  if(mount("/bin", "/tmp/bin", NULL, MS_BIND | MS_REC, NULL) == -1 \
  || mount("/lib", "/tmp/lib", NULL, MS_BIND | MS_REC, NULL) == -1 \
  || mount("/lib64", "/tmp/lib64", NULL, MS_BIND | MS_REC, NULL) == -1) {
    perror("mount: ");
    return 1;
  }
  // change root to the new root
  if(chroot("/tmp") == -1 || chdir("/") == -1) {
    perror("chroot: ");
    return 1;
  }
  // this is the new root fs
  printf("new root fs: %s\n", get_current_dir_name());
  // exec a shell
  
  char *shell_argv[] = {"/bin/sh", NULL};
  execv(shell_argv[0], shell_argv);
  perror("execv: ");
  return 1;
}

int main(int argc, char *argv[]) {
  char *p_stack = malloc(PS_STACK_SIZE);
  if (p_stack == NULL) {
    perror("malloc: ");
    exit(1);
  }
  char **child_fn_arg = (argc >= 2 ? &argv[1]: NULL); 
  pid_t pid = clone(child_fn, p_stack + PS_STACK_SIZE, CLONE_NEWPID | SIGCHLD, child_fn_arg);
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
