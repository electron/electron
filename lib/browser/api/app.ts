const bindings = process._linkedBinding('electron_browser_app');
const { app } = bindings;

// Only one app object permitted.
export default app;

// Routes the events to webContents.
const events = ['certificate-error', 'select-client-certificate'];
for (const name of events) {
  app.on(name as 'certificate-error', (event, webContents, ...args: any[]) => {
    // webContents is null for net module / utility process requests.
    if (webContents) webContents.emit(name, event, ...args);
  });
}

app._clientCertRequestPasswordHandler = null;
app.setClientCertRequestPasswordHandler = function (
  handler: (params: Electron.ClientCertRequestParams) => Promise<string>
) {
  app._clientCertRequestPasswordHandler = handler;
};

app.on(
  '-client-certificate-request-password',
  async (event: Electron.Event<Electron.ClientCertRequestParams>, callback: (password: string) => void) => {
    event.preventDefault();
    const { hostname, tokenName, isRetry } = event;
    if (!app._clientCertRequestPasswordHandler) {
      callback('');
      return;
    }
    const password = await app._clientCertRequestPasswordHandler({ hostname, tokenName, isRetry });
    callback(password);
  }
);
