// This is a temporary shim to aid in transition from the old
// BrowserWindow-based extensions stuff to the new native-backed extensions
// API.

import { app, session, BrowserWindow, deprecate } from 'electron/main';

app.whenReady().then(function () {
  const addExtension = function (srcDirectory: string) {
    return session.defaultSession.loadExtension(srcDirectory);
  };

  const removeExtension = function (name: string) {
    const extension = session.defaultSession.getAllExtensions().find(e => e.name === name);
    if (extension) { session.defaultSession.removeExtension(extension.id); }
  };

  const getExtensions = function () {
    const extensions: Record<string, any> = {};
    session.defaultSession.getAllExtensions().forEach(e => {
      extensions[e.name] = e;
    });
    return extensions;
  };

  BrowserWindow.addExtension = deprecate.moveAPI(addExtension, 'BrowserWindow.addExtension', 'session.loadExtension');
  BrowserWindow.removeExtension = deprecate.moveAPI(removeExtension, 'BrowserWindow.removeExtension', 'session.removeExtension');
  BrowserWindow.getExtensions = deprecate.moveAPI(getExtensions, 'BrowserWindow.getExtensions', 'session.getAllExtensions');
  BrowserWindow.addDevToolsExtension = deprecate.moveAPI(addExtension, 'BrowserWindow.addDevToolsExtension', 'session.loadExtension');
  BrowserWindow.removeDevToolsExtension = deprecate.moveAPI(removeExtension, 'BrowserWindow.removeDevToolsExtension', 'session.removeExtension');
  BrowserWindow.getDevToolsExtensions = deprecate.moveAPI(getExtensions, 'BrowserWindow.getDevToolsExtensions', 'session.getAllExtensions');
});
