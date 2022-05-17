#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void handle_prof_signal()
{
  printf("Done\n");
  exit(1);
}

void main()
{
  struct sigaction sig_action;
  memset(&sig_action, 0, sizeof(sig_action));
  sig_action.sa_sigaction = handle_prof_signal;
  sig_action.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sig_action.sa_mask);
  sigaction(SIGPROF, &sig_action, 0);

  struct itimerval timeout={0};
  timeout.it_value.tv_sec=1;
  setitimer(ITIMER_PROF, &timeout, 0);
  while (true){

  }
  return 0;
  //do { } while(1);
}
