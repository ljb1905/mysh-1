#include "signal_handlers.h"
#include <stdio.h>
#include <signal.h>

void catch_sigint(int signalNo)
{
  signal(SIGINT,SIG_IGN);
  return;
}

void catch_sigtstp(int signalNo)
{
  signal(SIGTSTP,SIG_IGN);
  return;
}

