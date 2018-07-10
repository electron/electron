'use strict'

const chai = require('chai')
const dirtyChai = require('dirty-chai')

const {expect} = chai
chai.use(dirtyChai)

const {remote} = require('electron')

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return

  this.timeout(3 * 60 * 1000)

  const {inAppPurchase} = remote

  it('canMakePayments() does not throw', () => {
    expect(() => {
      inAppPurchase.canMakePayments()
    }).to.not.throw()
  })

  it('finishAllTransactions() does not throw', () => {
    expect(() => {
      inAppPurchase.finishAllTransactions()
    }).to.not.throw()
  })

  it('finishTransactionByDate() does not throw', () => {
    expect(() => {
      inAppPurchase.finishTransactionByDate(new Date().toISOString())
    }).to.not.throw()
  })

  it('getReceiptURL() returns receipt URL', () => {
    const correctUrlEnd = inAppPurchase.getReceiptURL().endsWith('_MASReceipt/receipt')
    expect(correctUrlEnd).to.be.true()
  })

  it('purchaseProduct() fails when buying invalid product', done => {
    inAppPurchase.purchaseProduct('non-exist', 1, success => {
      expect(success).to.be.false()
      done()
    })
  })

  it('purchaseProduct() accepts optional arguments', done => {
    inAppPurchase.purchaseProduct('non-exist', () => {
      inAppPurchase.purchaseProduct('non-exist', 1)
      done()
    })
  })

  it('getProducts() returns an empty list when getting invalid product', done => {
    inAppPurchase.getProducts(['non-exist'], products => {
      expect(products).to.be.an('array').of.length(0)
      done()
    })
  })
})
