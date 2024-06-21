// Imported IPC Renderer
const { ipcRenderer } = require('electron');

document.addEventListener('DOMContentLoaded', () => {
   // Log message to confirm the script is loaded
  console.log('renderer.js loaded');

  // Get references to DOM elements
  const leftSidebar = document.getElementById('left-sidebar');
  const rightSidebar = document.getElementById('right-sidebar');
  const content = document.getElementById('content');

  // Update event listeners for left side
  document.getElementById('toggle-left-sidebar-button').addEventListener('click', () => {
    console.log('Left sidebar toggle button clicked');
    if (leftSidebar.style.transform === 'translateX(0%)') {
      leftSidebar.style.transform = 'translateX(-100%)';
      content.style.marginLeft = '0';
      ipcRenderer.send('resize-window', { side: 'left', action: 'close' });
    } else {
      leftSidebar.style.transform = 'translateX(0%)';
      content.style.marginLeft = '250px';
      ipcRenderer.send('resize-window', { side: 'left', action: 'open' });
    }
  });

  // Update event listeners for right side
  document.getElementById('toggle-right-sidebar-button').addEventListener('click', () => {
    console.log('Right sidebar toggle button clicked');
    if (rightSidebar.style.transform === 'translateX(0%)') {
      rightSidebar.style.transform = 'translateX(100%)';
      content.style.marginRight = '0';
      ipcRenderer.send('resize-window', { side: 'right', action: 'close' });
    } else {
      rightSidebar.style.transform = 'translateX(0%)';
      content.style.marginRight = '250px';
      ipcRenderer.send('resize-window', { side: 'right', action: 'open' });
    }
  });
});

