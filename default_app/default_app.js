const {app, BrowserWindow, contentTracing} = require('electron')
const path = require('path')

const options = {
  categoryFilter: '*',
  traceOptions: 'record-until-full,enable-sampling'
};

let mainWindow1 = null
let mainWindow2 = null
let mainWindow3 = null
let mainWindow4 = null
let mainWindow5 = null
let mainWindow6 = null
let mainWindow7 = null
let mainWindow8 = null

app.commandLine.appendSwitch('--disable-gpu');
app.commandLine.appendSwitch('--disable-gpu-compositing');

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    mainWindow1 = new BrowserWindow({
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true,
      webPreferences: {
        offScreen: true,
        nodeIntegration: false
      }
    })
    mainWindow1.loadURL(appUrl)
    mainWindow1.focus()
    mainWindow1.webContents.on('dom-ready', () => {
      mainWindow1.webContents.beginFrameSubscription(() => {
        console.log("asd")
      })
    })
    var start1, end1
    start1 = +new Date();
    mainWindow1.webContents.on('paint', (e, rect, w, h, data) => {
      end1 = +new Date();

      const d = end1 - start1
      console.log(`browser #1: ${d < 10 ? ` ${d}` : d} ms`)

      start1 = end1
    })
    
    /*contentTracing.startRecording(options, () => {
      console.log('Tracing started');

      setTimeout(() => {
        contentTracing.stopRecording('', (path) => {
          console.log('Tracing data recorded to ' + path);
        });
      }, 5000);
    });*/

    mainWindow2 = new BrowserWindow({
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true,
      webPreferences: {
        nodeIntegration: false
      }
    })
    mainWindow2.loadURL(appUrl)
    mainWindow2.focus()
    /*mainWindow2.webContents.on('dom-ready', () => {
      mainWindow2.webContents.beginFrameSubscription(() => {
        console.log("asd")
      })
    })
    
    var start2, end2
    start2 = +new Date();
    mainWindow2.webContents.on('paint', (e, rect, w, h, data) => {
      end2 = +new Date();
    
      const d = end2 - start2
      console.log(`browser #2: ${d < 10 ? ` ${d}` : d} ms`)
    
      start2 = end2
    })*/
  //   
  //   mainWindow3 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow3.loadURL(appUrl)
  //   mainWindow3.focus()
  //   mainWindow3.webContents.on('dom-ready', () => {
  //     mainWindow3.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start3, end3
  //   start3 = +new Date();
  //   mainWindow3.webContents.on('paint', (e, rect, w, h, data) => {
  //     end3 = +new Date();
  //   
  //     const d = end3 - start3
  //     console.log(`browser #3: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start3 = end3
  //   })
  //   
  //   mainWindow4 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow4.loadURL(appUrl)
  //   mainWindow4.focus()
  //   mainWindow4.webContents.on('dom-ready', () => {
  //     mainWindow4.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start4, end4
  //   start4 = +new Date();
  //   mainWindow4.webContents.on('paint', (e, rect, w, h, data) => {
  //     end4 = +new Date();
  //   
  //     const d = end4 - start4
  //     console.log(`browser #4: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start4 = end4
  //   })
  // 
  //   mainWindow5 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow5.loadURL(appUrl)
  //   mainWindow5.focus()
  //   mainWindow5.webContents.on('dom-ready', () => {
  //     mainWindow5.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start5, end5
  //   start5 = +new Date();
  //   mainWindow5.webContents.on('paint', (e, rect, w, h, data) => {
  //     end5 = +new Date();
  //   
  //     const d = end5 - start5
  //     console.log(`browser #5: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start5 = end5
  //   })
  //   
  //   mainWindow6 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow6.loadURL(appUrl)
  //   mainWindow6.focus()
  //   mainWindow6.webContents.on('dom-ready', () => {
  //     mainWindow6.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start6, end6
  //   start6 = +new Date();
  //   mainWindow6.webContents.on('paint', (e, rect, w, h, data) => {
  //     end6 = +new Date();
  //   
  //     const d = end6 - start6
  //     console.log(`browser #6: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start6 = end6
  //   })
  //   
  //   mainWindow7 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow7.loadURL(appUrl)
  //   mainWindow7.focus()
  //   mainWindow7.webContents.on('dom-ready', () => {
  //     mainWindow7.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start7, end7
  //   start7 = +new Date();
  //   mainWindow7.webContents.on('paint', (e, rect, w, h, data) => {
  //     end7 = +new Date();
  //   
  //     const d = end7 - start7
  //     console.log(`browser #7: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start7 = end7
  //   })
  //   
  //   mainWindow8 = new BrowserWindow({
  //     width: 800,
  //     height: 600,
  //     autoHideMenuBar: true,
  //     backgroundColor: '#FFFFFF',
  //     useContentSize: true,
  //     webPreferences: {
  //       nodeIntegration: false
  //     }
  //   })
  //   mainWindow8.loadURL(appUrl)
  //   mainWindow8.focus()
  //   mainWindow8.webContents.on('dom-ready', () => {
  //     mainWindow8.webContents.beginFrameSubscription(() => {
  //       console.log("asd")
  //     })
  //   })
  //   
  //   var start8, end8
  //   start8 = +new Date();
  //   mainWindow8.webContents.on('paint', (e, rect, w, h, data) => {
  //     end8 = +new Date();
  //   
  //     const d = end8 - start8
  //     console.log(`browser #8: ${d < 10 ? ` ${d}` : d} ms`)
  //   
  //     start8 = end8
  //   })
  })
}
