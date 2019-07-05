const fs = require('fs')

/**
 * Test sandbox environment used to fake network responses.
 */
class NetworkSandbox {
  constructor (protocol) {
    this.protocol = protocol
    this._resetFns = []
  }

  /**
   * Reset the sandbox.
   */
  async reset () {
    for (const resetFn of this._resetFns) {
      await resetFn()
    }
    this._resetFns = []
  }

  /**
   * Will serve the content of file at `filePath` to network requests towards
   * `protocolName` scheme.
   *
   * Example: `sandbox.serveFileFromProtocol('foo', 'index.html')` will serve the content
   * of 'index.html' to `foo://page` requests.
   *
   * @param {string} protocolName
   * @param {string} filePath
   */
  serveFileFromProtocol (protocolName, filePath) {
    return new Promise((resolve, reject) => {
      this.protocol.registerBufferProtocol(protocolName, (request, callback) => {
        // Disabled due to false positive in StandardJS
        // eslint-disable-next-line standard/no-callback-literal
        callback({
          mimeType: 'text/html',
          data: fs.readFileSync(filePath)
        })
      }, (error) => {
        if (error != null) {
          reject(error)
        } else {
          this._resetFns.push(() => this.unregisterProtocol(protocolName))
          resolve()
        }
      })
    })
  }

  unregisterProtocol (protocolName) {
    return new Promise((resolve, reject) => {
      this.protocol.unregisterProtocol(protocolName, (error) => {
        if (error != null) {
          reject(error)
        } else {
          resolve()
        }
      })
    })
  }
}

/**
 * Will create a NetworkSandbox that uses
 * `protocol` as `Electron.Protocol`.
 *
 * @param {Electron.Protocol} protocol
 */
function createNetworkSandbox (protocol) {
  return new NetworkSandbox(protocol)
}

exports.createNetworkSandbox = createNetworkSandbox
