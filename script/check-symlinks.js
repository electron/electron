const fs = require('fs');
const path = require('path');

const utils = require('./lib/utils');
const branding = require('../shell/app/BRANDING.json');

if (process.platform !== 'darwin') {
  console.log('Not checking symlinks on non-darwin platform');
  process.exit(0);
}

const appPath = path.resolve(__dirname, '..', '..', 'out', utils.getOutDir(), `${branding.product_name}.app`);
const visited = new Set();
const traverse = (p) => {
  if (visited.has(p)) return;

  visited.add(p);
  if (!fs.statSync(p).isDirectory()) return;

  for (const child of fs.readdirSync(p)) {
    const childPath = path.resolve(p, child);
    let realPath;
    try {
      realPath = fs.realpathSync(childPath);
    } catch (err) {
      if (err.path) {
        console.error('Detected an invalid symlink');
        console.error('Source:', childPath);
        let link = fs.readlinkSync(childPath);
        if (!link.startsWith('.')) {
          link = `../${link}`;
        }
        console.error('Target:', path.resolve(childPath, link));
        process.exit(1);
      } else {
        throw err;
      }
    }
    traverse(realPath);
  }
};

traverse(appPath);
