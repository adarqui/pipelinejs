/* pipelinejs -- adarqui && github.com/adarqui && adarq.org */

#include "pipelinejs.h"


PIPES pipes;
control_t * con;

Pipes::Pipes(void) {

	len = 0;
	pid = 0;
	pid_last = 0;

	fp = NULL;
	fp_mode = NULL;
	
	uv_stdout = NULL;
	uv_stderr = NULL;
	uv_ctrl = NULL;

	stdin = false;
	tty = false;

	tty_fd = 0;
	tty_pfds[0] = 0;
	tty_pfds[1] = 0;

	bytes_written = 0;
	pgrp = 0;
	
}

Pipes::~Pipes(void) {
}


void erase_pid(PIPES::iterator pi) {

	uv_poll_stop(pi->uv_stdout);
	uv_poll_stop(pi->uv_stderr);
	uv_poll_stop(pi->uv_ctrl);

	close(pi->stdout[0]);
	close(pi->stdout[1]);
	close(pi->stderr[0]);
	close(pi->stderr[1]);
	close(pi->ctrl[0]);
	close(pi->ctrl[1]);

	if(pi->tty_pfds[0] > 0) close(pi->tty_pfds[0]);
	if(pi->tty_pfds[1] > 0) close(pi->tty_pfds[1]);

	if(pi->fp != NULL) {
		fclose(pi->fp);
		pi->fp = NULL;
	}

	pipes.erase(pi);

	return;
}



void sigchld_handler(int num) {
	/*
	 * Reap dead children
	 */

	PIPES::iterator pi;

	pid_t pid;
	int status = 0;

	pid = wait(&status);

	if(!WIFEXITED(status)) return;

	control_write(CONTROL_EXIT, pid);
	return;
}


void sigchld_handler_exit(int num) {

	/* Test: just exit */

	exit(0);
}


void emit(PIPES::iterator pi, pipes_t *pi2, char * type, char * buf, int len, pid_t pid) {

	Handle<Value> argv[1];

	Local<Object> data = Object::New();
	data->Set(String::New("type"), String::New(type));
	data->Set(String::New("data"), String::New(buf));
	data->Set(String::New("len"), Integer::NewFromUnsigned(len));
	data->Set(String::New("pid"), Integer::NewFromUnsigned(pid));

	argv[0] = data;

	TryCatch try_catch;

	if(pi2 != NULL) {
		pi2->cb->Call(Context::GetCurrent()->Global(), 1, argv);
	} else {
		pi->cb->Call(Context::GetCurrent()->Global(), 1, argv);
	}

	if(try_catch.HasCaught()) {
		FatalException(try_catch);
	}

	return;
}


void on_handle_close(uv_handle_t *handle) {
	delete handle;
}


void control_write(int type, int arg) {

	size_t n;
	control_data_t * cond = new control_data_t;

	cond->type = type;
	cond->arg = arg;

	n = write(con->pfds[1], cond, sizeof(control_data_t));

	return;
}


control_data_t * control_read(void) {
	control_data_t * cond = new control_data_t;

	char buf[10024];
	size_t n;

	n = read(con->pfds[0], buf, sizeof(control_data_t));	
	if(n<=0) { 
		return NULL;
	}


	cond = (control_data_t *) buf;
	return cond;
}

void on_control_event(uv_poll_t * handle, int status, int events) {

	PIPES::iterator pi;
	control_data_t * cond;
	pid_t pid;

	cond = control_read();

	pid = cond->arg;

	if(cond->type == CONTROL_EXIT) {

		for(pi = pipes.begin(); pi != pipes.end(); pi++) {
			if(pi->pid_last == pid) {
				emit(pi, NULL, (char *)strdup("exit"), (char *)strdup("hi"), 0, pid);
				erase_pid(pi);
				kill(pid, SIGTERM);
				break;
			}
		}
	}

	return;
}

void on_fd_event(uv_poll_t *handle, int status, int events) {
	HandleScope scope;
	char buf[10024], *type_string;
	size_t n;

	fd_wrapper_t * fdw = (fd_wrapper_t *)handle->data;

	n = read(fdw->fd, buf, sizeof(buf)-2);
	buf[n] = '\0';

	if(fdw->p->fp) {
		write(fileno(fdw->p->fp), buf, n);
	}


	switch(fdw->type) {
		case FDW_STDOUT: {
			type_string = (char *)"stdout";
			break;
		}
		case FDW_STDERR: {
			type_string = (char *)"stderr";
			break;
		}
		case FDW_CTRL: {
			type_string = (char *)"ctrl";
			break;
		}
		default: {
			type_string = (char *)"unknown";
			break;
		}
	}

	PIPES::iterator it;
	emit(it, fdw->p, type_string, buf, n, fdw->p->pid);
}


void dumpit(void) {
    PIPES::iterator i;

	return;

	printf("dumping structs:\n");
    for(i = pipes.begin(); i != pipes.end(); i++) {
		PROC::iterator j;

		printf("\ttest: %i\n", i->len);

		for(j = i->procs.begin(); j != i->procs.end(); j++) {
			printf("\t\ttest2: pid=%i fd=%i ::: child pid=%i\n", i->pid, i->stdout[1], j->pid);
			int k;
			for(k=0; /*j->argv[k]!=NULL*/ k<j->argc; k++) {
				printf("\t\t\t%s\n", j->argv[k]);
			}
		}
    }

    return;
}


void add_fd(PIPES::iterator pi, int type) {
	uv_poll_t * _h;
	fd_wrapper_t * fdw = new fd_wrapper_t;
	int err;

	/* DO NOT SET O_NONBLOCK. HELL. */

	fdw->type = type;

	switch(type) {
		case FDW_STDOUT: {
			fdw->fd = pi->stdout[0];
			pi->uv_stdout = new uv_poll_t;
			_h = pi->uv_stdout;
			break;
		}
		case FDW_STDERR: {
			fdw->fd = pi->stderr[0];
			pi->uv_stderr = new uv_poll_t;
			_h = pi->uv_stderr;
			break;
		}
		case FDW_CTRL: {
			fdw->fd = pi->ctrl[0];
			pi->uv_ctrl = new uv_poll_t;
			_h = pi->uv_ctrl;
			break;
		}
		default: {
			return;
		}
	}


	fdw->p	= (pipes_t *)&*pi;
	_h->data= fdw;
	
	err = uv_poll_init(uv_default_loop(), _h, fdw->fd);
    err = uv_poll_start(_h, UV_READABLE, on_fd_event);

	return;
}


void pipe_op(int id, int op) {
	
	PIPES::iterator pi;

	if(id < 2 || op < 0) return;

	for(pi = pipes.begin(); pi != pipes.end(); pi++) {
		if(pi->pid == id) {

			switch(op) {
				case OP_STOP: {
					kill(pi->pid, SIGSTOP);
					return;
				}
				case OP_START: {
					kill(pi->pid, SIGCONT);
					return;
				}
				case OP_KILL: {
					kill(pi->pid, SIGTERM);
					erase_pid(pi);
					return;
				}
				case OP_STATUS: {
					break;
				}
				default: {
					return;
				}
			}
		}
	}
}



void runit(PIPES::iterator pi) {

	PROC::iterator i;

	pipe(pi->stdout);
	pipe(pi->stderr);
	pipe(pi->ctrl);

	add_fd(pi, FDW_STDOUT);
	add_fd(pi, FDW_STDERR);
	add_fd(pi, FDW_CTRL);

	i = pi->procs.begin();
	execit(pi, i, pi->procs.end(), pi->procs.begin());

	return;
}


int execit(PIPES::iterator pi, PROC::iterator i, PROC::iterator end, PROC::iterator begin) {

	PROC::iterator o;

	pid_t pid;
	int prev_fd;
	int pip[2];


	if(i == end) { exit(0); }


	prev_fd = -1;

	if(pi->stdin == true) {
		/* Turn initial proc's stdin into a pipe */
		prev_fd = pi->ctrl[0];
	}

	for(i = begin; i != end;  ) {

		pip[1] = -1;

		o = i;
		i++;


		if(i != end) {
			pipe(pip);
		}

		pid = fork();
		if(!pid) {
			if(o == begin) {
				/* first proc */
//FIXME
				setpgid(getpid(), 0);
			}
			else {
//FIXME
				setpgid(pi->pgrp, 0);
			}

			if(i != end) {
				close(0);

				if (pip[1] >= 0) {
					close(pip[0]);
				}
				if (prev_fd > 0) {
					dup2(prev_fd, 0);
					close(prev_fd);
				}
				if (pip[1] > 1) {
					dup2(pip[1], 1);
					close(pip[1]);
				}
			}
			else {
				close(0);

				if(prev_fd > 0) {
					dup2(prev_fd,0);
					close(prev_fd);
				}

				if(pi->tty == true) {

					pi->tty_fd = make_tty(pi->stdout[1]);
					dup2(pi->tty_fd, 1);
	
				}
				else {
					close(1);
					dup2(pi->stdout[1], 1);
					close(2);
					dup2(pi->stderr[1], 2);
				}
			}

			execvp(o->argv[0], o->argv);
			exit(0);
			/* no ret */
		}


		if(o == begin) {
			/* First fork */
			pi->pgrp = pid;	
			pi->pid = pid;
		}

		o->pid = pid;
		pi->pid_last = pid;

		if (prev_fd >= 0 && pi->stdin == false)
			close(prev_fd);

		prev_fd = pip[0];
		/* Don't want to trigger debugging */
		if (pip[1] != -1)
			close(pip[1]);

	}

	return 0;
}




Handle<Value> ctrl(const Arguments& args) {

	HandleScope scope;
	int identifier = -1, n = 0;
	char * my_data = NULL, * my_type = NULL;
	PIPES::iterator pi;

	if(args.Length() < 1) {
		return ThrowException(Exception::TypeError(String::New("Error: Not enough arguments")));
	}

    if (args[0]->IsObject()) {
        Handle<Object> opts = Handle<Object>::Cast(args[0]);

        if(opts->Has(String::New("id"))) {
            /* Test object */
            if(opts->Get(String::New("id"))->IsInt32()) {
				Handle<Value> i = opts->Get(String::New("id"));
				identifier = i->Int32Value();
            }
        }
		if(opts->Has(String::New("type"))) {
			if(opts->Get(String::New("type"))->IsString()) {
				String::Utf8Value type(opts->Get(String::New("type")));
				my_type = strdup(*type);
			}
		}
		if(opts->Has(String::New("data"))) {
			if(opts->Get(String::New("data"))->IsString()) {
				String::Utf8Value data(opts->Get(String::New("data")));
				my_data = strdup(*data);
			}
		}
	}


	if(identifier < 2) {
		return ThrowException(Exception::TypeError(String::New("Error: Invalid identifier")));
	}

	if(my_type == NULL) {
		return ThrowException(Exception::TypeError(String::New("Error: Invalid type")));
	}
	
	if(!strcasecmp(my_type, "stdin")) {

		if(my_data == NULL) {
			return ThrowException(Exception::TypeError(String::New("Error: Invalid data")));
		}

		for(pi = pipes.begin(); pi != pipes.end(); pi++) {
			if(pi->pid == identifier) {
				if(!strcasecmp(my_type, "stdin")) {
					n = write(pi->ctrl[1], my_data, strlen(my_data));
					pi->bytes_written += n;
				}
				break;
			}
		}
	}
	else if(!strcasecmp(my_type, "stop")) {
		pipe_stop(identifier);
	}
	else if(!strcasecmp(my_type, "start")) {
		pipe_start(identifier);
	}
	else if(!strcasecmp(my_type, "kill")) {
		pipe_kill(identifier);
	}

	return scope.Close(Integer::New(1));
}


Handle<Value> exec(const Arguments& args) {
	/*
	 * exec(opts, argvs, envp, cb)
	 */
	HandleScope scope;

	PIPES::iterator it;
	class Pipes pi;


	if(args.Length() < 4) {
		return ThrowException(Exception::TypeError(String::New("Error: Incorrect number of arguments")));
	}

	pi.len = args.Length();
	pi.cb = Persistent<Function>::New(Local<Function>::Cast(args[3]));


	/*
	 * Argument parsing
	 */
	if (args[0]->IsObject()) {

		Handle<Object> opts = Handle<Object>::Cast(args[0]);
		
		if(opts->Has(String::New("tty"))) {
			if(opts->Get(String::New("tty"))->IsString()) {
				String::Utf8Value tty(opts->Get(String::New("tty")));
				if(!strcasecmp(*tty, "true")) {
					pi.tty = true;
				}
				else {
					pi.tty = false;
				}
			}
		}
		if(opts->Has(String::New("stdin"))) {
			if(opts->Get(String::New("stdin"))->IsString()) {
				String::Utf8Value _stdin(opts->Get(String::New("stdin")));
				if(!strcasecmp(*_stdin, "true")) {
					pi.stdin = true;
				} else {
					pi.stdin = false;
				}
			}
		}
		if(opts->Has(String::New("mode"))) {
			if(opts->Get(String::New("mode"))->IsString()) {
				String::Utf8Value mode(opts->Get(String::New("mode")));
				pi.fp_mode = *mode;
			}
		}
		if(opts->Has(String::New("file"))) {
			if(opts->Get(String::New("file"))->IsString()) {
				String::Utf8Value file(opts->Get(String::New("file")));
				pi.fp = fopen(*file, pi.fp_mode);
				if(pi.fp==NULL) {
					return ThrowException(Exception::TypeError(String::New("Error: Opening log file")));
				}
			}
		}
	}


	/*
	 * Set up a new chain of pipes and insert them
	 */
	if (args[1]->IsArray()) {
		Local<Array> a = Array::Cast(*args[1]);
		for (int index = 0, size = a->Length(); index < size; index++) {
			proc_t p;

			p.argv = NULL;
			p.argc = 0;
			p.envp = NULL;
			p.pid = 0;

			Local<Value> element = a->Get(index);
			if (element->IsArray()) {
				Local<Array> b = Array::Cast(*element);
				// do useful stuff with b

				int y = 0;
				int argc = b->Length();

				if(argc <= 0) { puts("exec: argc <= 0"); return scope.Close(Integer::New(1)); }

				int argv_length = argc + 1;
				p.argv = new char*[argv_length];
				p.argc=0;
				p.argv[argv_length-1] = NULL;
				for (y = 0; y < argc; y++) {
					String::Utf8Value arg(b->Get(Integer::New(y))->ToString());
					p.argv[y] = strdup(*arg);
					p.argc += 1;
				}

				pi.procs.insert(pi.procs.end(), p);
			}

		}

		it = pipes.insert(pipes.end(), pi);

		dumpit();
		runit(it);

		return scope.Close(Integer::New(it->pid));
	}


	return scope.Close(Integer::New(-1));
}


void setup_control_socket(void) {

	uv_poll_t * _h = new uv_poll_t;
	con = new control_t;
	int err;

    /* DO NOT SET O_NONBLOCK. HELL. */

	pipe(con->pfds);

	_h->data = con;
	err = uv_poll_init(uv_default_loop(), _h, con->pfds[0]);
	err = uv_poll_start(_h, UV_READABLE, on_control_event);

	return;
}



void init(Handle<Object> exports, Handle<Object> module) {
	NODE_SET_METHOD(exports, "exec", exec);
	NODE_SET_METHOD(exports, "ctrl", ctrl);

	clear_cloexec(0);
	clear_cloexec(1);
	clear_cloexec(2);

	signal(SIGCHLD, sigchld_handler);

	setup_control_socket();
}


NODE_MODULE(pipelinejs, init)
