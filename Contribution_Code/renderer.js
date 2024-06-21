const { ipcRenderer } = require('electron');

document.addEventListener('DOMContentLoaded', () => {
  console.log('renderer.js loaded');

  const leftSidebar = document.getElementById('left-sidebar');
  const rightSidebar = document.getElementById('right-sidebar');
  const content = document.getElementById('content');

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

  // Event listeners for right sidebar option buttons
  document.querySelectorAll('.option-button').forEach(button => {
    button.addEventListener('click', (event) => {
      console.log(`${event.target.parentElement.querySelector('.option-text').innerText} button clicked`);
      // Handle the specific action for each button here
      // For example, open a new window, show a list, etc.
    });
  });
});


    window.electronAPI.toggleRightSidebar();
    rightOpen = !rightOpen;
});
