/* pipelinejs -- adarqui */

#include "pipelinejs.h"

void err(int n, char * s) {
	puts(s);
	exit(n);
}

int clear_cloexec(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFD, 0);
	if(flags < 0) return -1;

	flags &= ~FD_CLOEXEC;
	return fcntl(fd, F_SETFD, flags);
}

int make_tty(int xfd) {

	pid_t pid = 0, forkpid;

	int terminalfd;
	char buf[255];

	pid = forkpty(&terminalfd, NULL, NULL, NULL);
	if (pid < 0)
	{
		exit(0);
	}
	else if(pid == 0) {
		/* .. */
    }
/*
	else {
		exit(0);
	}
*/


	if(pid) {
		forkpid = fork();
		if(forkpid == 0){
			while (1) {
				int numbytes = read(terminalfd, buf, 254);
				if (numbytes <= 0){
					exit(0);
				}
 
				if(write(xfd, buf, numbytes)<=0) exit(0);;
			}
		} else {
		}
	}

	return terminalfd;
}


void pipe_stop(int id) {

    pipe_op(id, OP_STOP);
}

void pipe_start(int id) {

    pipe_op(id, OP_START);
}

void pipe_status(int id) {
    pipe_op(id, OP_START);
}

void pipe_kill(int id) {
    pipe_op(id, OP_KILL);
}

