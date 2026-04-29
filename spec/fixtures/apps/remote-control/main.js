// eslint-disable-next-line camelcase
const electron_1 = require('electron');

// eslint-disable-next-line camelcase
const { app } = electron_1;
const http = require('node:http');
// eslint-disable-next-line camelcase,@typescript-eslint/no-unused-vars
const promises_1 = require('node:timers/promises');
const v8 = require('node:v8');

function getAutoQuitTimeout () {
  const argPrefix = '--remote-app-timeout=';
  const arg = process.argv.find((arg) => arg.startsWith(argPrefix));

  if (arg) {
    const timeout = parseInt(arg.slice(argPrefix.length), 10);

    if (Number.isSafeInteger(timeout) && timeout > 0) {
      return timeout;
    }
  }

  return 30000;
}

if (app.commandLine.hasSwitch('boot-eval')) {
  // eslint-disable-next-line no-eval
  eval(app.commandLine.getSwitchValue('boot-eval'));
}

app.whenReady().then(() => {
  const server = http.createServer((req, res) => {
    const chunks = [];
    req.on('data', chunk => { chunks.push(chunk); });
    req.on('end', () => {
      const js = Buffer.concat(chunks).toString('utf8');
      (async () => {
        try {
          const result = await Promise.resolve(eval(js)); // eslint-disable-line no-eval
          res.end(v8.serialize({ result }));
        } catch (e) {
          res.end(v8.serialize({ error: e.stack }));
        }
      })();
    });
  }).listen(0, '127.0.0.1', () => {
    process.stdout.write(`Listening: ${server.address().port}\n`);
  });
});

setTimeout(() => {
  process.exit(0);
}, getAutoQuitTimeout());
