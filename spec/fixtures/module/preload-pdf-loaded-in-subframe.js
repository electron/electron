const {ipcRenderer} = require('electron')

document.addEventListener("DOMContentLoaded", function(event) {
  var subframe = document.getElementById('pdf-frame');
  if (subframe) {
    subframe.contentWindow.addEventListener('pdf-loaded', function (event) {
	  ipcRenderer.send('pdf-loaded', event.detail)
	});
  }
});

