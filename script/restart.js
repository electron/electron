const childProcess  = require('child_process');
const path          = require('path');

const pid          = process.argv[2];
var timeout        = process.argv[3];
const systemOs     = process.platform;
var startupCmd     = '';
const waitForExit  = function(pid, timeout, cb) {
  waitTime = timeout
  var checkPid = setInterval(function() {
    if (waitTime > 0) {
      try {
        process.kill(pid, 0);
      } catch(e) {
        clearInterval(checkPid);
        cb(true);
      }
    } else {
      clearInterval(checkPid);
      cb(false);
    }

    waitTime -= 1;
  }, timeout * 1000);
};

for (var i = 4; i < process.argv.length; i++) {
  startupCmd = startupCmd + ' ' + (path.resolve(process.argv[i]));
}

if (timeout < 0)
  timeout = 10;
else if (timeout > 300)
  timeout = 300;

waitForExit(pid, timeout, function(success) {
  if (success) {
    childProcess.exec(startupCmd, function(err) {
      if (err) process.exit(1);
      process.exit();
    });
  } else {
    // timeout exceeded
    process.exit(1);
  }
});
