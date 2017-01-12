'use strict'

const {ipcRenderer} = require('electron')

const {guestInstanceId, openerId} = process
const hiddenPage = process.argv.includes('--hidden-page')

require('./window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage)
