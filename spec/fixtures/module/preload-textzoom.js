const { webFrame, ipcRenderer } = require('electron');
const url = require('url');
const { textZoom } = url.parse(window.location.href, true).query;

webFrame.setTextZoomFactor(parseFloat(textZoom));

ipcRenderer.send('initalTextZoomFactor', webFrame.getTextZoomFactor());
