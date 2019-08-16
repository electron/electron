const { WebContentsView, app, webContents } = require('electron')
console.log('In leak-exit-webcontentsview.js')
app.on('ready', function () {
  console.log('In leak-exit-webcontentsview.js ready')
  const web = webContents.create({})
  console.log('In leak-exit-webcontentsview.js done webContents create')
  new WebContentsView(web)  // eslint-disable-line
  console.log('In leak-exit-webcontentsview.js done new WebContentsView(web)')
  process.nextTick(() => {
    console.log('In leak-exit-webcontentsview.js about to call quit')
    app.quit()
  })
})
