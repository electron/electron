const { app, net, session } = require('electron');

if (process.env.TEST_DUMP_FILE) {
  app.commandLine.appendSwitch('log-net-log', process.env.TEST_DUMP_FILE);
}

function request () {
  return new Promise((resolve) => {
    const req = net.request(process.env.TEST_REQUEST_URL);
    req.on('response', () => {
      resolve();
    });
    req.end();
  });
}

app.whenReady().then(async () => {
  const netLog = session.defaultSession.netLog;

  if (process.env.TEST_DUMP_FILE_DYNAMIC) {
    await netLog.startLogging(process.env.TEST_DUMP_FILE_DYNAMIC);
  }

  await request();

  if (process.env.TEST_MANUAL_STOP) {
    await netLog.stopLogging();
  }

  app.quit();
});
