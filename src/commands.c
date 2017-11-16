#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

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

char *pos[6] = { "/usr/local/bin/", "/usr/bin/", "usr/sbin/", "/bin/", "/sbin/" };

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
  if (n_commands > 0) {
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
      if(!access(com->argv[0],0))
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
         // execv(com->argv[0],com->argv);
          for(int i=0;i<20;i++)
{
printf("%d\n",i);
sleep(1);
}
exit(1);
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
        for(int i=0;i<6;i++)
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
