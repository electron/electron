'use strict';

const { app } = require('electron');
const path = require('path');

const getTempDirectory = function () {
  try {
    return app.getPath('temp');
  } catch {
    // Delibrately laze-load the os module, this file is on the hot
    // path when booting Electron and os takes between 5 - 8ms to load and we do not need it yet
    return require('os').tmpdir();
  }
};

exports.crashReporterInit = function (options) {
  const productName = options.productName || app.name;
  const crashesDirectory = path.join(getTempDirectory(), `${productName} Crashes`);

  return {
    productName,
    crashesDirectory,
    appVersion: app.getVersion()
  };
};
