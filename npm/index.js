const fs = require('fs');
const path = require('path');
const { Menu } = require('electron');

const pathFile = path.join(__dirname, 'path.txt');

function getElectronPath() {

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);

  let executablePath;
  if (fs.existsSync(pathFile)) {
    executablePath = fs.readFileSync(pathFile, 'utf-8');
  }
  if (process.env.ELECTRON_OVERRIDE_DIST_PATH) {
    return path.join(process.env.ELECTRON_OVERRIDE_DIST_PATH, executablePath || 'electron');
  }
  if (executablePath) {
    return path.join(__dirname, 'dist', executablePath);
  } else {
    throw new Error('Electron failed to install correctly, please delete node_modules/electron and try installing again');
  }
}

module.exports = getElectronPath();
