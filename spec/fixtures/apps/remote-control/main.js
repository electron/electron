// eslint-disable-next-line camelcase
const electron_1 = require('electron');

// eslint-disable-next-line camelcase
const { app } = electron_1;
const http = require('node:http');
// eslint-disable-next-line camelcase,@typescript-eslint/no-unused-vars
const promises_1 = require('node:timers/promises');
const v8 = require('node:v8');
const math = require('mathjs');

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
          // Evaluate the expression safely using mathjs. Only mathematical expressions are supported.
          let result;
          try {
            result = math.evaluate(js);
          } catch (err) {
            res.end(v8.serialize({ error: 'Invalid expression: ' + err.message }));
            return;
          }
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
}, 30000);
