#ifndef PIPELINEJS_MISC_H
#define PIPELINEJS_MISC_H

#include "pipelinejs.h"


void err(int, char *);
int clear_cloexec(int);
int make_tty(int);

void pipe_stop(int);
void pipe_start(int);
void pipe_status(int);
void pipe_kill(int);

#endif
