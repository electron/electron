import { expect } from 'chai'
import * as http from 'http'
import * as qs from 'querystring'
import * as path from 'path'
import { session, WebContents, webContents } from 'electron'
import { AddressInfo } from 'net'

const fixturesPath = path.resolve(__dirname, 'fixtures')

describe('webRequest module', () => {
  const ses = session.defaultSession
  const server = http.createServer((req, res) => {
    if (req.url === '/serverRedirect') {
      res.statusCode = 301
      res.setHeader('Location', 'http://' + req.rawHeaders[1])
      res.end()
    } else {
      res.setHeader('Custom', ['Header'])
      let content = req.url
      if (req.headers.accept === '*/*;test/header') {
        content += 'header/received'
      }
      if (req.headers.origin === 'http://new-origin') {
        content += 'new/origin'
      }
      res.end(content)
    }
  })
  let defaultURL: string

  before((done) => {
    server.listen(0, '127.0.0.1', () => {
      const port = (server.address() as AddressInfo).port
      defaultURL = `http://127.0.0.1:${port}/`
      done()
    })
  })

  after(() => {
    server.close()
  })

  let contents: WebContents = null as unknown as WebContents
  // NB. sandbox: true is used because it makes navigations much (~8x) faster.
  before(async () => {
    contents = (webContents as any).create({ sandbox: true })
    await contents.loadFile(path.join(fixturesPath, 'pages', 'jquery.html'))
  })
  after(() => (contents as any).destroy())

  async function ajax (url: string, options = {}) {
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`)
  }

  describe('webRequest.onBeforeRequest', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRequest(null)
    })

    it('can cancel the request', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({
          cancel: true
        })
      })
      await expect(ajax(defaultURL)).to.eventually.be.rejectedWith('404')
    })

    it('can filter URLs', async () => {
      const filter = { urls: [defaultURL + 'filter/*'] }
      ses.webRequest.onBeforeRequest(filter, (details, callback) => {
        callback({ cancel: true })
      })
      const { data } = await ajax(`${defaultURL}nofilter/test`)
      expect(data).to.equal('/nofilter/test')
      await expect(ajax(`${defaultURL}filter/test`)).to.eventually.be.rejectedWith('404')
    })

    it('receives details object', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        expect(details.id).to.be.a('number')
        expect(details.timestamp).to.be.a('number')
        expect(details.webContentsId).to.be.a('number')
        expect(details.url).to.be.a('string').that.is.equal(defaultURL)
        expect(details.method).to.be.a('string').that.is.equal('GET')
        expect(details.resourceType).to.be.a('string').that.is.equal('xhr')
        expect(details.uploadData).to.be.undefined()
        callback({})
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/')
    })

    it('receives post data in details object', async () => {
      const postData = {
        name: 'post test',
        type: 'string'
      }
      ses.webRequest.onBeforeRequest((details, callback) => {
        expect(details.url).to.equal(defaultURL)
        expect(details.method).to.equal('POST')
        expect(details.uploadData).to.have.lengthOf(1)
        const data = qs.parse(details.uploadData[0].bytes.toString())
        expect(data).to.deep.equal(postData)
        callback({ cancel: true })
      })
      await expect(ajax(defaultURL, {
        type: 'POST',
        data: postData
      })).to.eventually.be.rejectedWith('404')
    })

    it('can redirect the request', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: `${defaultURL}redirect` })
        } else {
          callback({})
        }
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/redirect')
    })

    it('does not crash for redirects', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: false })
      })
      await ajax(defaultURL + 'serverRedirect')
      await ajax(defaultURL + 'serverRedirect')
    })
  })

  describe('webRequest.onBeforeSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onBeforeSendHeaders(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        expect(details.requestHeaders).to.be.an('object')
        expect(details.requestHeaders['Foo.Bar']).to.equal('baz')
        callback({})
      })
      const { data } = await ajax(defaultURL, { headers: { 'Foo.Bar': 'baz' } })
      expect(data).to.equal('/')
    })

    it('can change the request headers', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders
        requestHeaders.Accept = '*/*;test/header'
        callback({ requestHeaders: requestHeaders })
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/header/received')
    })

    it('can change CORS headers', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders
        requestHeaders.Origin = 'http://new-origin'
        callback({ requestHeaders: requestHeaders })
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/new/origin')
    })

    it('resets the whole headers', async () => {
      const requestHeaders = {
        Test: 'header'
      }
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders })
      })
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(requestHeaders)
      })
      await ajax(defaultURL)
    })
  })

  describe('webRequest.onSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onSendHeaders(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.be.an('object')
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/')
    })
  })

  describe('webRequest.onHeadersReceived', () => {
    afterEach(() => {
      ses.webRequest.onHeadersReceived(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK')
        expect(details.statusCode).to.equal(200)
        expect(details.responseHeaders!['Custom']).to.deep.equal(['Header'])
        callback({})
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/')
    })

    it('can change the response header', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!
        responseHeaders['Custom'] = ['Changed'] as any
        callback({ responseHeaders: responseHeaders })
      })
      const { headers } = await ajax(defaultURL)
      expect(headers).to.match(/^custom: Changed$/m)
    })

    it('can change CORS headers', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!
        responseHeaders['access-control-allow-origin'] = ['http://new-origin'] as any
        callback({ responseHeaders: responseHeaders })
      })
      const { headers } = await ajax(defaultURL)
      expect(headers).to.match(/^access-control-allow-origin: http:\/\/new-origin$/m)
    })

    it('does not change header by default', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        callback({})
      })
      const { data, headers } = await ajax(defaultURL)
      expect(headers).to.match(/^custom: Header$/m)
      expect(data).to.equal('/')
    })

    it('follows server redirect', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders
        callback({ responseHeaders: responseHeaders })
      })
      const { headers } = await ajax(defaultURL + 'serverRedirect')
      expect(headers).to.match(/^custom: Header$/m)
    })

    it('can change the header status', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders
        callback({
          responseHeaders: responseHeaders,
          statusLine: 'HTTP/1.1 404 Not Found'
        })
      })
      const { headers } = await contents.executeJavaScript(`new Promise((resolve, reject) => {
        const options = {
          ...${JSON.stringify({ url: defaultURL })},
          success: (data, status, request) => {
            reject(new Error('expected failure'))
          },
          error: (xhr) => {
            resolve({ headers: xhr.getAllResponseHeaders() })
          }
        }
        $.ajax(options)
      })`)
      expect(headers).to.match(/^custom: Header$/m)
    })
  })

  describe('webRequest.onResponseStarted', () => {
    afterEach(() => {
      ses.webRequest.onResponseStarted(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onResponseStarted((details) => {
        expect(details.fromCache).to.be.a('boolean')
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK')
        expect(details.statusCode).to.equal(200)
        expect(details.responseHeaders!['Custom']).to.deep.equal(['Header'])
      })
      const { data, headers } = await ajax(defaultURL)
      expect(headers).to.match(/^custom: Header$/m)
      expect(data).to.equal('/')
    })
  })

  describe('webRequest.onBeforeRedirect', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRedirect(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', async () => {
      const redirectURL = defaultURL + 'redirect'
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: redirectURL })
        } else {
          callback({})
        }
      })
      ses.webRequest.onBeforeRedirect((details) => {
        expect(details.fromCache).to.be.a('boolean')
        expect(details.statusLine).to.equal('HTTP/1.1 307 Internal Redirect')
        expect(details.statusCode).to.equal(307)
        expect(details.redirectURL).to.equal(redirectURL)
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/redirect')
    })
  })

  describe('webRequest.onCompleted', () => {
    afterEach(() => {
      ses.webRequest.onCompleted(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onCompleted((details) => {
        expect(details.fromCache).to.be.a('boolean')
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK')
        expect(details.statusCode).to.equal(200)
      })
      const { data } = await ajax(defaultURL)
      expect(data).to.equal('/')
    })
  })

  describe('webRequest.onErrorOccurred', () => {
    afterEach(() => {
      ses.webRequest.onErrorOccurred(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: true })
      })
      ses.webRequest.onErrorOccurred((details) => {
        expect(details.error).to.equal('net::ERR_BLOCKED_BY_CLIENT')
      })
      await expect(ajax(defaultURL)).to.eventually.be.rejectedWith('404')
    })
  })
})
