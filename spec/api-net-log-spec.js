const chai = require('chai')
const dirtyChai = require('dirty-chai')
const http = require('http')
const fs = require('fs')
const os = require('os')
const path = require('path')
const ChildProcess = require('child_process')
const { remote } = require('electron')
const { session } = remote
const appPath = path.join(__dirname, 'fixtures', 'api', 'net-log')
const dumpFile = path.join(os.tmpdir(), 'net_log.json')
const dumpFileDynamic = path.join(os.tmpdir(), 'net_log_dynamic.json')

const { expect } = chai
chai.use(dirtyChai)
const isCI = remote.getGlobal('isCi')
const netLog = session.fromPartition('net-log').netLog

describe('netLog module', () => {
  let server
  const connections = new Set()

  before(done => {
    server = http.createServer()
    server.listen(0, '127.0.0.1', () => {
      server.url = `http://127.0.0.1:${server.address().port}`
      done()
    })
    server.on('connection', (connection) => {
      connections.add(connection)
      connection.once('close', () => {
        connections.delete(connection)
      })
    })
    server.on('request', (request, response) => {
      response.end()
    })
  })

  after(done => {
    for (const connection of connections) {
      connection.destroy()
    }
    server.close(() => {
      server = null
      done()
    })
  })

  afterEach(() => {
    try {
      if (fs.existsSync(dumpFile)) {
        fs.unlinkSync(dumpFile)
      }
      if (fs.existsSync(dumpFileDynamic)) {
        fs.unlinkSync(dumpFileDynamic)
      }
    } catch (e) {
      // Ignore error
    }
  })

  it('should begin and end logging to file when .startLogging() and .stopLogging() is called', done => {
    expect(netLog.currentlyLogging).to.be.false()
    expect(netLog.currentlyLoggingPath).to.equal('')

    netLog.startLogging(dumpFileDynamic)

    expect(netLog.currentlyLogging).to.be.true()
    expect(netLog.currentlyLoggingPath).to.equal(dumpFileDynamic)

    netLog.stopLogging((path) => {
      expect(netLog.currentlyLogging).to.be.false()
      expect(netLog.currentlyLoggingPath).to.equal('')

      expect(path).to.equal(dumpFileDynamic)
      expect(fs.existsSync(dumpFileDynamic)).to.be.true()

      done()
    })
  })

  it('should silence when .stopLogging() is called without calling .startLogging()', done => {
    expect(netLog.currentlyLogging).to.be.false()
    expect(netLog.currentlyLoggingPath).to.equal('')

    netLog.stopLogging(path => {
      expect(netLog.currentlyLogging).to.be.false()
      expect(netLog.currentlyLoggingPath).to.equal('')

      expect(path).to.equal('')

      done()
    })
  })

  it('should begin and end logging automatically when --log-net-log is passed', done => {
    if (isCI && process.platform === 'linux') {
      done()
      return
    }

    const appProcess = ChildProcess.spawn(remote.process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: server.url,
          TEST_DUMP_FILE: dumpFile
        }
      })

    appProcess.once('exit', () => {
      expect(fs.existsSync(dumpFile)).to.be.true()
      done()
    })
  })

  it('should begin and end logging automtically when --log-net-log is passed, and behave correctly when .startLogging() and .stopLogging() is called', done => {
    if (isCI && process.platform === 'linux') {
      done()
      return
    }

    const appProcess = ChildProcess.spawn(remote.process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: server.url,
          TEST_DUMP_FILE: dumpFile,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic,
          TEST_MANUAL_STOP: true
        }
      })

    appProcess.once('exit', () => {
      expect(fs.existsSync(dumpFile)).to.be.true()
      expect(fs.existsSync(dumpFileDynamic)).to.be.true()
      done()
    })
  })

  it('should end logging automatically when only .startLogging() is called', done => {
    if (isCI && process.platform === 'linux') {
      done()
      return
    }

    const appProcess = ChildProcess.spawn(remote.process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: server.url,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic
        }
      })

    appProcess.once('close', () => {
      expect(fs.existsSync(dumpFileDynamic)).to.be.true()
      done()
    })
  })
})
