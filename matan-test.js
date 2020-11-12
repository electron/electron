var spawn = require('child_process').spawn;
var cp = spawn('cmd.exe', ['/c', 'whoami']);

cp.stdout.on("data", function(data) {
    console.log(data.toString());
});

cp.stderr.on("data", function(data) {
    console.error(data.toString());
});

cp.on('close', function (code) {
    console.log('child process exited with code ' + code);
});