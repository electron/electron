const https = require('https');
const fs = require('fs');
const path = require('path');
const { BrowserWindow } = require('electron');

const exec = require('util').promisify(require('child_process').exec);

async function executeCommand (command) {
  const { stdout, stderr } = await exec(command);
  console.log(`$ ${command}`);
  console.log(`1> ${stdout.trim()}`);
  console.log(`2> ${stderr.trim()}`);
  return stdout;
}

async function runInBrowserWindow (script) {
  const browserWindow = new BrowserWindow({
    webPreferences: { nodeIntegration: true, contextIsolation: false, show: false },
    show: true
  });
  await browserWindow.loadFile(__filename);
  await browserWindow.webContents.openDevTools();
  await browserWindow.webContents.executeJavaScript(`(${script})()`);
}

function startServer () {
  return new Promise((resolve, reject) => {
    const options = {
      key: fs.readFileSync(path.join(__dirname, 'certificates', 'localhost.decrypted.key')),
      cert: fs.readFileSync(path.join(__dirname, 'certificates', 'localhost.crt'))
    };
    const server = https.createServer(options, (req, res) => {
      res.writeHead(200);
      res.end('hello world\n');
    });
    server.listen(0, () => {
      resolve(server);
    });
  });
}

async function testScriptInBrowserWindow () {
  const { ipcRenderer } = require('electron');

  return new Promise((resolve, reject) => {
    const https = require('https');
    https.get(
      process.env.serverUrl,
      { rejectUnauthorized: true },
      (res) => {
        ipcRenderer.send('message-from-renderer', { status: res.statusCode, error: null });
        resolve();
      }
    ).on('error', (e) => {
      // '500' is the custom error status code, since we are asserting on statusCode '200' in our test to be passed
      ipcRenderer.send('message-from-renderer', { status: '500', error: e });
      reject(e);
    });
  });
}

async function testScriptInMainProcess () {
  return new Promise((resolve, reject) => {
    const https = require('https');
    https.get(
      process.env.serverUrl,
      { rejectUnauthorized: true },
      (res) => {
        resolve(res.statusCode);
      }
    ).on('error', (e) => {
      reject(e);
    });
  });
}

async function testZscalerIntegration (electronTest) {
  const statusCodes = [];
  const registryPath = path.resolve(__dirname, 'certificates', 'CA.pem');

  // Install the root certificate in windows store
  await executeCommand(`certutil -addstore root "${registryPath}"`);

  // Start the https server with certificate generated from root certificate
  const server = await startServer();
  process.env.serverUrl = `https://127.0.0.1:${server.address().port}`;

  const { app, ipcMain } = require('electron');
  await app.whenReady();

  if (app.isReady()) {
    ipcMain.once('message-from-renderer', (event, value) => {
      statusCodes.push(value.status);
    });
  }

  await runInBrowserWindow(testScriptInBrowserWindow);
  statusCodes.push(await testScriptInMainProcess());

  server.close();
  return statusCodes;
}

module.exports.testZscalerIntegration = testZscalerIntegration;
