EventEmitter = require('events').EventEmitter

BrowserWindow = process.atomBinding('window').BrowserWindow
BrowserWindow.prototype.__proto__ = EventEmitter.prototype

module.exports = BrowserWindow
