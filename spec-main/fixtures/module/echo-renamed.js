let echo;
try {
  echo = require('echo');
} catch (e) {
  process.exit(1);
}
process.exit(echo(0));
