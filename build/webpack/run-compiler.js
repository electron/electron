const fs = require('fs');
const path = require('path');
const webpack = require('webpack');

const configPath = process.argv[2];
const outPath = path.resolve(process.argv[3]);
const config = require(configPath);
config.output = {
  path: path.dirname(outPath),
  filename: path.basename(outPath)
};

const { wrapInitWithProfilingTimeout } = config;
delete config.wrapInitWithProfilingTimeout;

webpack(config, (err, stats) => {
  if (err) {
    console.error(err);
    process.exit(1);
  } else if (stats.hasErrors()) {
    console.error(stats.toString('normal'));
    process.exit(1);
  } else {
    if (wrapInitWithProfilingTimeout) {
      const contents = fs.readFileSync(outPath, 'utf8');
      const newContents = `function ___electron_webpack_init__() {
${contents}
};
if ((globalThis.process || binding.process).argv.includes("--profile-electron-init")) {
  setTimeout(___electron_webpack_init__, 0);
} else {
  ___electron_webpack_init__();
}`;
      fs.writeFileSync(outPath, newContents);
    }
    process.exit(0);
  }
});
