/* pipelinejs -- adarqui */
#ifndef PIPELINEJS_H
#define PIPELINEJS_H

#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>
#include <v8.h>
#include <pcap.h>

#include <errno.h>
#include <pty.h>
#include <termios.h>

#include <list>

#include "pipelinejs_misc.h"

using namespace v8;
using namespace node;
using namespace std;

typedef struct proc {
	char ** argv;
	char ** envp;
	int argc;
	pid_t pid;
} proc_t;

typedef list<proc_t> PROC;

class Pipes {
	public:
		PROC procs;
		Persistent<Function> cb;
		int len;

		int stdout[2];
		int stderr[2];
		int ctrl[2];

		pid_t pid;
		pid_t pid_last;

		uv_poll_t * uv_stdout;
		uv_poll_t * uv_stderr;
		uv_poll_t * uv_ctrl;	

		bool stdin;

		bool tty;
		int tty_fd;
		int tty_pfds[2];

		FILE * fp;
		char * fp_mode;

		int bytes_written;

		pid_t pgrp;

		Pipes();
		~Pipes();

		void Clean (void);
};

typedef Pipes pipes_t;

typedef list<pipes_t> PIPES;

enum fd_wrappers_types {
	FDW_STDOUT,
	FDW_STDERR,
	FDW_CTRL,
};

typedef struct fd_wrapper {
	int type;
	int fd;
	pipes_t * p;
} fd_wrapper_t;


enum control_stuff {
	 CONTROL_EXIT
};

typedef struct control {
	int pfds[2];
} control_t;

typedef struct control_data {
	int type;
	int arg;
} control_data_t;


extern int errno;

void init(Handle<Object> exports, Handle<Object> module);
Handle<Value> exec(const Arguments&);
Handle<Value> ctrl(const Arguments&);

void dumpit(void);
void runit(PIPES::iterator);
int execit(PIPES::iterator, PROC::iterator, PROC::iterator, PROC::iterator);

int clear_cloexec(int);

enum pipe_ops {
	OP_STOP,
	OP_START,
	OP_STATUS,
	OP_KILL
};
void pipe_op(int, int);

void erase_pid(PIPES::iterator);
void add_fd(PIPES::iterator, int);
void on_handle_close(uv_handle_t *);
void on_fd_event(uv_poll_t *, int, int);

void emit(PIPES::iterator, pipes_t *, char *, char *, int, pid_t);

void setup_control_socket(void);
void on_control_event(uv_poll_t *, int, int);
void control_write(int,int);
control_data_t * control_read(void);

/*
 * List stuff
 */

#endif
