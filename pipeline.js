/* pipelinejs -- adarqui && github.com/adarqui && adarq.org */

var deps = {
	pl	: require('./build/Release/pipelinejs'),
	ev	: require('events'),
}

console.log("loaded.");

var test = function() {
    var argv1 = [ "ls", "-al" ];
	var argv2 = [ "wc", "-l" ];

	var p = new deps.ev.EventEmitter();

	p.pipe = deps.pl.exec(function(res) {
		p.emit("data", res);
	}, argv1, argv2);

	p.on('data', function(res) {
		console.log("p data", res);
	})

	p.on('error', function(re) {
		console.log("p error", res);
	})

	p.on('end', function(res) {
		console.log("p end", res);
	})
}


var exec = function(o) {
	/*
	 * o {
	 * 	opts
	 *	argvs
	 *	envp
	 * ...
	 */

	var p = new deps.ev.EventEmitter();

	p.pipe = deps.pl.exec(o.opts, o.argvs, o.envp, function(res) {
		switch(res.type) {
			case 'exit': {
				p.emit('exit', res);
				return;
			}
			default:
				break;
		}
		p.emit("data", res);
	});

	if(p.pipe < 2) return null;

	p.on('stdin', function(data) {
		deps.pl.ctrl({ id: p.pipe, type: "stdin", data: data });
	});

	p.on('stop', function(data) {
		deps.pl.ctrl({ id: p.pipe, type: "stop", data: null });
	});

	p.on('start', function(data) {
		deps.pl.ctrl({ id: p.pipe, type: "start", data: null });
	});

	p.on('kill', function(data) {
		deps.pl.ctrl({ id: p.pipe, type: "kill", data: null });
	});


	return p;
}

exports.exec = exec;
