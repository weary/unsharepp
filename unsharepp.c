

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)
#if 0 // enable/disable logging
#define LOGV(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOGV(...)
#endif

// stack size for clone-child
#define STACK_SIZE (40 * 1024)

static int drop_privileges(const char *who)
{
  uid_t target = getuid();
  if (target == 0) {
    LOGV("%s not dropping privileges, running as root\n", who);
    return 0;
  }
  if (setuid(target) != 0) {
    LOG("setuid: %s unable to drop user privileges: %s\n", who,
        strerror(errno));
    return 1;
  }
  return 0;
}

static int remount_proc()
{
  LOGV("remounting /proc\n");

  if (mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
    LOG("cannot change root filesystem to private: %s\n", strerror(errno));
    return 1;
  }

  if (mount("none", "/proc", NULL, MS_REC | MS_PRIVATE, NULL)) {
    LOG("failed to make /proc mounted private: %s\n", strerror(errno));
    return 1;
  }

  if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL)) {
    LOG("failed to mount /proc: %s\n", strerror(errno));
    return 1;
  }
  return 0;
}

static int child(void *arg)
{
  char **argv = (char **)arg;

  if (getpid() != 1) {
    LOG("error: Child is not pid 1\n");
    return 1;
  }

  if (remount_proc())
    return 1;

  if (drop_privileges("child"))
    return 1;

  LOGV("launching %s\n", argv[0]);
  if (execvp(argv[0], argv) != 0) {
    LOG("failed to execvp: %s\n", strerror(errno));
    return 1;
  }
  LOG("child somehow returned from execvp\n");
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc <= 1 || strcmp(argv[1], "--help") == 0) {
    LOG("\nrun a command in its own pid namespace. needs setuid bit to work, "
        "command is run as getuid() user\n\n");
    LOG("usage: %s <command> <args>\n", argv[0]);
    return 1;
  }

  if (geteuid() != 0) {
    LOG("%s is not run as root\n", argv[0]);
    return 1;
  }

  char *stack = malloc(STACK_SIZE);
  if (stack == NULL) {
    LOG("failed to allocate stack\n");
    return 1;
  }
  char *stack_top = stack + STACK_SIZE;

  pid_t pid = clone(child, stack_top, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
      (void *)(argv + 1));
  if (pid == -1) {
    LOG("clone failed: %s\n", strerror(errno));
    return 1;
  }

  LOGV("%s waiting for child pid %d\n", argv[0], pid);
  if (drop_privileges("master"))
    return 1;

  // wait for child to terminate
  int status = 0;
  waitpid(pid, &status, 0);

  LOGV("%s regained control, child terminated with exit status %d\n", argv[0],
      WEXITSTATUS(status));
  return WEXITSTATUS(status);
}
