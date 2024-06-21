const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

let mainWindow; // Declare the mainWindow variable globally
const sidebarWidth = 250; // Define the width of the sidebar

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'src', 'preload.js'), // Use the correct path for preload.js
      contextIsolation: false, // Allow context isolation for security
      nodeIntegration: true // Enable Node.js integration in the renderer process
    }
  });

  mainWindow.loadFile('src/index.html'); // Load the HTML file from the correct path
}

// Simplified app event handling
app.on('ready', createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit(); // Quit the app when all windows are closed, except on macOS
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow(); // Re-create a window when the app is activated and no windows are open
  }
});

// Improved sidebar toggle handling
ipcMain.on('resize-window', (event, { side, action }) => {
  try {
    const [currentWidth, currentHeight] = mainWindow.getSize();
    let targetWidth = action === 'open' ? currentWidth + sidebarWidth : currentWidth - sidebarWidth;

    function resize() {
      const [width, height] = mainWindow.getSize();
      if ((action === 'open' && width < targetWidth) || (action === 'close' && width > targetWidth)) {
        mainWindow.setSize(width + (action === 'open' ? 10 : -10), height);
        requestAnimationFrame(resize); // Use requestAnimationFrame for smooth resizing
      } else {
        mainWindow.setSize(targetWidth, height);
      }
    }

    resize();
  } catch (error) {
    console.error('Error resizing window:', error); // Catch and log any errors during resizing
  }
});

