let echo;
try {
  echo = require('@electron-ci/echo');
} catch {
  process.exit(1);
}
process.exit(echo(0));
