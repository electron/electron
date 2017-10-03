'use strict'

const {ipcRenderer} = require('electron')

const {guestInstanceId, openerId} = process
const hiddenPage = process.argv.includes('--hidden-page')
const usesNativeWindowOpen = process.argv.includes('--native-window-open')

require('./window-setup')(ipcRenderer, guestInstanceId, openerId, hiddenPage, usesNativeWindowOpen)
