'use strict'

const {ipcMain} = require('electron')
const {EventEmitter} = require('events')
const {BrowserView} = process.atomBinding('browser_view')
const v8Util = process.atomBinding('v8_util')

Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype)

module.exports = BrowserView
