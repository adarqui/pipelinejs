/* pipelinejs test code -- adarqui && github.com/adarqui && adarq.org */
var app = app || {};

app.deps = {
	pipes	: require("./pipeline.js"),
	fs		: require('fs'),
	kp		: require('keypress'),
}

app.conf = {

	term : {
	},

	input : {
		buf	: [],
	},
	opts : {
		stdin	: "false",
		tty		: "false",
		str		: "str",
		file	: null,
	},
	p : {},
	p_stdin : {},
	p_stdin_int : {},
	p_cur : null,
 
}

app.fn = {

	handle : {
		stdin_test: function() {

            var o = {
                opts : {
                    tty     : app.conf.opts.tty,
                    file    : app.conf.opts.file,
                    stdin   : "true",
                    str     : "str",
                },
                argvs : [],
                envp : {
                },
            }


			o.argvs.push(['grep', '--line-buffered', '-i', 'he']);

            app.conf.p_stdin = app.deps.pipes.exec(o);
            app.conf.p_stdin.on('data', function(res) {
                console.log("p_stdin data:", res);
            });

			if(app.conf.p_stdin == null) return;

			app.conf.p_stdin_int = setInterval(function() {
				app.conf.p_stdin.emit('stdin', 'hello!\n');
			}, 5000);

			app.conf.p[app.conf.p_stdin.pipe] = app.conf.p_stdin;

		},

		input : function() {

			if(app.conf.input.buf.length == 0) app.conf.input.buf = [];

			app.conf.input.buf = app.conf.input.buf.join('');

//console.log("buf:", app.conf.input.buf);

			/*
			 * TEST COMMANDS - for testing features of .exec
			 */
			var buf = app.conf.input.buf;

			if(buf.indexOf("#help") == 0) {
				console.log("#tty, #file </path/to/file>, #stop <pid>, #start <pid>, #kill <pid>");
				app.conf.input.buf = [];
				return;
			}

			if(buf.indexOf("#tty") == 0) {
				if(app.conf.opts.tty == "true") {
					console.log("# Disabling tty");
					app.conf.opts.tty = "false";
				}
				else {
					console.log("# Enabling tty");
					app.conf.opts.tty = "true";
				}
				app.conf.input.buf = [];
				return;
			}
			if(buf.indexOf("#file") == 0) {
				app.conf.opts.file = buf.substring(6);
				if(app.conf.opts.file.length > 0) {
					console.log("# Logging to file: ", app.conf.opts.file);
					app.conf.opts.file = app.conf.opts.file.replace(/ /g, '');
				} else {
					console.log("# Disabling file logging");
					app.conf.opts.file = null;
				}
				app.conf.input.buf = [];
				return;
			}

			if(buf.indexOf("#stop") == 0) {
				var pid = buf.substring(6);
				console.log("# Stopping pid=", pid);
				var p = app.conf.p[pid];
				if(p != undefined) {
					p.emit('stop', {});
				}
				app.conf.input.buf = [];
				return;
			}

			if(buf.indexOf("#start") == 0) {
				var pid = buf.substring(7);
				console.log("# Starting pid=", pid);	
				var p = app.conf.p[pid];
				if(p != undefined) {
					p.emit('start', {});
				}
				app.conf.input.buf = [];
				return;
			}

			if(buf.indexOf("#kill") == 0) {
				var pid = buf.substring(6);
				console.log("# Killing pid=", pid);
				var p = app.conf.p[pid];
				if(p != undefined) {
					p.emit('kill', {});
				}
				app.conf.input.buf = [];
				return;
			}

			var o = {
				opts : {
					tty		: app.conf.opts.tty,
					file	: app.conf.opts.file,
					stdin	: app.conf.opts.stdin,
					str		: "str",
				},
				argvs : [],
				envp : {
				},
			}

			var args = app.conf.input.buf.split('|');
			for(var v in args) {
				var y = args[v].split(' ');
				y = y.filter(function(s) {
					if(s.length) return true;
				});
				o.argvs.push(y);
			}

//console.log(o.argvs);



			var p = app.deps.pipes.exec(o);
			if(p == null) return;


			app.conf.p[p.pipe] = p;
			app.conf.p[p.pipe].on('data', function(res) {
				var lines = res.data.split('\n');
				console.log("p data:", lines);
			});

			app.conf.p[p.pipe].on('exit', function(res) {
// console.log("PIPE EXITED!! ", res);
				delete app.conf.p[res.pid];

			});

			app.conf.p_cur = p.pipe;
			app.conf.input.buf = [];

		},
	},
	init: function() {

		app.fn.handle.stdin_test();

        app.deps.kp(process.stdin);

        process.stdin.on('end', function() {
            console.log("END: ", app.conf.p);
			for(var v in app.conf.p) {
				app.conf.p[v].emit('kill', {});
			}
			app.conf.p_stdin.emit('kill', {});

            process.exit(0);
        });

        process.stdin.on('keypress', function (ch, key) {

            var ret = 0;

            if(ch == undefined) {
                app.conf.term.fd.write(key.sequence);
                return;
            }

			if(!ret) {
				if(ch == '\n' || ch == '\r') {
					/* leave newline crap up to parse1 */
					app.fn.handle.input();
				}
				else {
					if(! (app.conf.input.buf.length > 0) ) app.conf.input.buf = [];
					app.conf.input.buf.push(ch);
				}
            }

        });

	},
}



app.fn.init();
