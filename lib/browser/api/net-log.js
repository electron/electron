'use strict'

const { netLog, NetLog } = process.atomBinding('net_log')

NetLog.prototype.stopLogging = function (callback) {
  if (callback && typeof callback !== 'function') {
    throw new Error('Invalid callback function')
  }

  const path = this.currentlyLoggingPath
  this._stopLogging(() => {
    if (callback) callback(path)
  })
}

module.exports = netLog
