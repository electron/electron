const repl = require('node:repl');
const { Writable } = require('node:stream');

const r = repl.start();

r.output = new Writable({ write () { } });

const sleep = async (ms) => new Promise(resolve => setTimeout(resolve, ms));

async function main () {
  let x = 0;
  while (x < 50) {
    await sleep(0);
    r.write('(new Function(\'import("")\'))()\n');
    x += 1;
  }
  process.exit(0);
}

main();
