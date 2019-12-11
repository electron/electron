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
})
