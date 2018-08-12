const { expect } = require('chai')

describe('the test suite', () => {
  it('should pass with chai assertions', () => {
    expect(true).to.equal(true)
  })

  it('should fail on async assertions', async () => {
    await new Promise((resolve) => setTimeout(resolve, 100))
    expect(false).to.equal(true)
  })

  it('should pass with chai assertions 2', async () => {

  })

  describe('with a child suite', () => {
    it('should also run a test', () => {
      expect(true).to.equal(true)
    })
  })
})
