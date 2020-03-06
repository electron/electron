const { expect } = require('chai')
const { webFrame } = require('electron')

describe('webFrame module', function () {
  it('top is self for top frame', () => {
    expect(webFrame.top.context).to.equal(webFrame.context)
  })

  it('opener is null for top frame', () => {
    expect(webFrame.opener).to.be.null()
  })

  it('firstChild is null for top frame', () => {
    expect(webFrame.firstChild).to.be.null()
  })

  it('getFrameForSelector() does not crash when not found', () => {
    expect(webFrame.getFrameForSelector('unexist-selector')).to.be.null()
  })

  it('findFrameByName() does not crash when not found', () => {
    expect(webFrame.findFrameByName('unexist-name')).to.be.null()
  })

  it('findFrameByRoutingId() does not crash when not found', () => {
    expect(webFrame.findFrameByRoutingId(-1)).to.be.null()
  })

  describe('executeJavaScript', () => {
    let childFrameElement, childFrame

    before(() => {
      childFrameElement = document.createElement('iframe')
      document.body.appendChild(childFrameElement)
      childFrame = webFrame.firstChild
    })

    after(() => {
      childFrameElement.remove()
    })

    it('executeJavaScript() yields results via a promise and a sync callback', done => {
      let callbackResult, callbackError

      childFrame
        .executeJavaScript('1 + 1', (result, error) => {
          callbackResult = result
          callbackError = error
        })
        .then(
          promiseResult => {
            expect(promiseResult).to.equal(2)
            done()
          },
          promiseError => {
            done(promiseError)
          }
        )

      expect(callbackResult).to.equal(2)
      expect(callbackError).to.be.undefined()
    })

    it('executeJavaScriptInIsolatedWorld() yields results via a promise and a sync callback', done => {
      let callbackResult, callbackError

      childFrame
        .executeJavaScriptInIsolatedWorld(999, [{ code: '1 + 1' }], (result, error) => {
          callbackResult = result
          callbackError = error
        })
        .then(
          promiseResult => {
            expect(promiseResult).to.equal(2)
            done()
          },
          promiseError => {
            done(promiseError)
          }
        )

      expect(callbackResult).to.equal(2)
      expect(callbackError).to.be.undefined()
    })

    it('executeJavaScript() yields errors via a promise and a sync callback', done => {
      let callbackResult, callbackError

      childFrame
        .executeJavaScript('thisShouldProduceAnError()', (result, error) => {
          callbackResult = result
          callbackError = error
        })
        .then(
          promiseResult => {
            done(new Error('error is expected'))
          },
          promiseError => {
            expect(promiseError).to.be.an('error')
            done()
          }
        )

      expect(callbackResult).to.be.undefined()
      expect(callbackError).to.be.an('error')
    })

    // executeJavaScriptInIsolatedWorld is failing to detect exec errors and is neither
    // rejecting nor passing the error to the callback. This predates the reintroduction
    // of the callback so will not be fixed as part of the callback PR
    // if/when this is fixed the test can be uncommented.
    //
    // it('executeJavaScriptInIsolatedWorld() yields errors via a promise and a sync callback', done => {
    //   let callbackResult, callbackError
    //
    //   childFrame
    //     .executeJavaScriptInIsolatedWorld(999, [{ code: 'thisShouldProduceAnError()' }], (result, error) => {
    //       callbackResult = result
    //       callbackError = error
    //     })
    //     .then(
    //       promiseResult => {
    //         done(new Error('error is expected'))
    //       },
    //       promiseError => {
    //         expect(promiseError).to.be.an('error')
    //         done()
    //       }
    //     )
    //
    //   expect(callbackResult).to.be.undefined()
    //   expect(callbackError).to.be.an('error')
    // })

    it('executeJavaScript(InIsolatedWorld) can be used without a callback', async () => {
      expect(await webFrame.executeJavaScript('1 + 1')).to.equal(2)
      expect(await webFrame.executeJavaScriptInIsolatedWorld(999, [{ code: '1 + 1' }])).to.equal(2)
    })
  })
})
