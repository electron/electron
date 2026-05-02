app.whenReady().then(() => {
  
  const payload = { appPath: app.getAppPath() };
  console.log(JSON.stringify(payload));
  app.quit();
  
});
