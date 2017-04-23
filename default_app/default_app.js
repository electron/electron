const {app, BrowserWindow, Notification} = require('electron')
const path = require('path')

let mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    const options = {
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      webPreferences: {
        nodeIntegrationInWorker: true
      },
      useContentSize: true
    }
    if (process.platform === 'linux') {
      options.icon = path.join(__dirname, 'icon.png')
    }

    mainWindow = new BrowserWindow(options)
    mainWindow.loadURL(appUrl)
    mainWindow.focus()

    const n = new Notification({
      title: 'Hello World',
      body: 'This is the long and complicated body for this notification that just goes on and on and on and never really seems to stop',
      silent: true,
      icon: '/Users/samuel/Downloads/ninja.png',
      hasReply: true,
      replyPlaceholder: 'Type Here!!'
    });
    n.on('show', () => console.log('showed'));
    n.on('click', () => console.info('clicked!!'));
    n.on('reply', (e, reply) => console.log('Replied:', reply));

    n.show();
  })
}
