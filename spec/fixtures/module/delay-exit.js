const { app } = require('electron')

process.on('message', () => app.quit())
