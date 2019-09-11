#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>

#define FORKSRV_FD 198

int main(int argc, char **argv) {

  uint8_t  tmp[4];
  uint32_t pid;
  uint8_t* min_target = argv[1];
  uint8_t* gen_file = argv[2];
  uint8_t* arguments[] = { min_target, gen_file, NULL };
  uint32_t bytes = 0;

  int st_pipe[2], ctl_pipe[2];
  
  if (pipe(st_pipe) || pipe(ctl_pipe)) { printf("pipe() failed"); exit(-1); }

  pid = fork();

  if (pid < 0) { printf("fork() failed"); exit(-1); }

  if (!pid) {

      struct rlimit r;

      if (!getrlimit(RLIMIT_NOFILE, &r) && r.rlim_cur < FORKSRV_FD + 2) {

          r.rlim_cur = FORKSRV_FD + 2;
          setrlimit(RLIMIT_NOFILE, &r); /* Ignore errors */

      }

      if (dup2(ctl_pipe[0], FORKSRV_FD) < 0) { printf("dup2() failed"); exit(-1); }
      if (dup2(st_pipe[1], FORKSRV_FD + 1) < 0) { printf("dup2() failed"); exit(-1); }

      close(ctl_pipe[0]);
      close(ctl_pipe[1]);
      close(st_pipe[0]);
      close(st_pipe[1]);

      execv(min_target, (char**) arguments);

  } else {

      close(ctl_pipe[0]);
      close(st_pipe[1]);

      if (read(st_pipe[0], &tmp, 4) != 4) { printf("read() failed"); exit(-1); }

      while (1) {

        char enter = 0;
        while (enter != '\r' && enter != '\n') {
          printf("\nPress ENTER to continue...");
          enter = getchar();
        }

        if (write(ctl_pipe[1], &tmp, 4) != 4) { printf("write() failed"); exit(-1); }

        if (read(st_pipe[0], &bytes, 4) != 4) { printf("read() failed"); exit(-1); }

        printf("\ngot bytes: %u", bytes);

      }

  }

}