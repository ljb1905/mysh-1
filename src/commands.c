#define SOCK_PATH "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>

#include "commands.h"
#include "built_in.h"

char **inp;
int childpid;
int l;

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

char *pos[5] = {
  "/usr/local/bin/",
  "/usr/bin/",
  "/usr/sbin/",
  "/bin/",
  "/sbin/" };

void *clientf(struct single_command (*in)[512])
{
//
//printf("2\n");
  int c_sock,len,rc;
  struct sockaddr_un c_sockadd;
  struct sockaddr_un s_sockadd;
  memset(&c_sockadd, 0, sizeof(struct sockaddr_un));
  memset(&s_sockadd, 0, sizeof(struct sockaddr_un));

  c_sock = socket(AF_UNIX, SOCK_STREAM,0);
  if(c_sock==-1)
  {
    printf("SOCKET ERROR\n"); // check
    exit(1);
  }
  c_sockadd.sun_family = AF_UNIX;
  strcpy(c_sockadd.sun_path, CLIENT_PATH);
  len = sizeof(c_sockadd);
  unlink(CLIENT_PATH);

  rc = bind(c_sock, (struct sockaddr *)&c_sockadd,len);
  if(rc==-1)
  {
    printf("BIND ERROR\n");//check
    close(c_sock);
    exit(1);
  }
  s_sockadd.sun_family = AF_UNIX;
  strcpy(s_sockadd.sun_path, SERVER_PATH);
//
//printf("2\n");
  int chk = connect(c_sock,(struct sockaddr *)&s_sockadd,len);
  if(chk==-1) {
  printf("CONNECTION ERROR\n");
  close(c_sock);
  exit(1);
  }
  int tmp = dup(STDOUT_FILENO);
  dup2(c_sock, STDOUT_FILENO);
  close(c_sock);
  evaluate_command(1,in);
  close(STDOUT_FILENO);
  dup2(tmp,STDOUT_FILENO);
  close(c_sock);
  close(tmp);
//
//printf("2\n");
  pthread_exit(NULL);
}

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if(n_commands >= 2) {
//
//printf("1\n");
      void *status;
      struct single_command in[512];
      struct single_command out[512];
      memcpy(in,&((*commands)[0]),sizeof(struct single_command));
      memcpy(out,&((*commands)[1]),sizeof(struct single_command));

      int len,s_sock,c_sock;
      struct sockaddr_un s_add,c_add;
      pthread_t thr;

      memset(&s_add,0,sizeof(struct sockaddr_un));
      memset(&c_add,0,sizeof(struct sockaddr_un));
      s_sock = socket(AF_UNIX, SOCK_STREAM,0);
      if(s_sock ==-1)
      {
        printf("SOCKET ERROR!\n");
        exit(1);
      }
      s_add.sun_family = AF_UNIX;
      strcpy(s_add.sun_path,SOCK_PATH);
      len = sizeof(s_add);

      unlink(SOCK_PATH);

      int rc = bind(s_sock, (struct sockaddr*)&s_add,sizeof(s_add));
      if(rc==-1)
      {
        printf("BIND ERROR!\n");
        close(s_sock);
        exit(1);
      }
      int thread1 = pthread_create(&thr,NULL,(void*)clientf,&in);
      if(thread1<0)
      {
        printf("THREAD ERROR!\n");
        exit(1);
      }
      rc = listen(s_sock,5);
      if(rc==-1){
        printf("LISTEN ERROR!\n");
        close(s_sock);
        exit(1);
      }
      c_sock = accept(s_sock,(struct sockaddr*)&c_add,&len);
      pthread_join(thr,&status);
//
//printf("1\n");
      int tmp = dup(STDIN_FILENO);
      dup2(c_sock,STDIN_FILENO);
      evaluate_command(1,&out);
      close(STDIN_FILENO);
      dup2(tmp,STDIN_FILENO);
      close(c_sock);
      close(tmp);
      close(s_sock);
//
//printf("1\n");
      return 0;
  }

  else if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {
      if(!access(com->argv[0],0) && n_commands == 1)
      {
        pid_t par = fork();
        int status;
        int idx=(com->argc)-1;
        //child & &x
        if(!par && strcmp(com->argv[idx],"&"))
          execv(com->argv[0],com->argv);
        //child & &o
        else if(!par && !strcmp(com->argv[idx],"&"))
        {
          printf("pid : %d\n",getpid());
          com->argv[idx]=0;//&reset
          execv(com->argv[0],com->argv);
/*for(int i=0;i<20;i++)
{
printf("%d\n",i);
sleep(1);
}
exit(1);*/
        }
        //par & &x
        else if(par && strcmp(com->argv[idx],"&")) 
          wait(&status);
        //par & &o
        else if(par && !strcmp(com->argv[idx],"&"))
        {
          printf("start\n");
          l = com->argc -1;
          inp = (char**)malloc(com->argc*sizeof(char*));
          for(int i=0;i<l;i++)
          {
            int size = strlen(com->argv[i]);
            inp[i] = (char*)malloc(sizeof(char)*(size+1));
            strcpy(inp[i],com->argv[i]);
          }
          childpid = par;
          return 0;
        }
        else return -1;
      }
      //extra
      else
      {
//
//printf("sajdklf\n");
        for(int i=0;i<5;i++)
        {
          char test[200];
          strcpy(test,pos[i]);
          strcat(test, com->argv[0]);
          if(!access(test,0))
          {
            strcpy(com->argv[0],test);
            int par = fork();
            int status;
            if(par)
            {
              wait(&status);
              return 0;
            }
            else execv(com->argv[0],com->argv);
          }
        }
      fprintf(stderr, "%s: command not found\n", com->argv[0]);
      }
      return -1;
    }
  }

  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
