'use strict'

const assert = require('assert')

const {remote} = require('electron')

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return

  this.timeout(3 * 60 * 1000)

  const {inAppPurchase} = remote

  it('canMakePayments() does not throw', () => {
    inAppPurchase.canMakePayments()
  })

  it('finishAllTransactions() does not throw', () => {
    inAppPurchase.finishAllTransactions()
  })

  it('finishTransactionByDate() does not throw', () => {
    inAppPurchase.finishTransactionByDate(new Date().toISOString())
  })

  it('getReceiptURL() returns receipt URL', () => {
    assert.ok(inAppPurchase.getReceiptURL().endsWith('_MASReceipt/receipt'))
  })

  it('purchaseProduct() fails when buying invalid product', (done) => {
    inAppPurchase.purchaseProduct('non-exist', 1, (success) => {
      assert.ok(!success)
      done()
    })
  })

  it('purchaseProduct() accepts optional arguments', (done) => {
    inAppPurchase.purchaseProduct('non-exist', () => {
      inAppPurchase.purchaseProduct('non-exist', 1)
      done()
    })
  })

  it('getProducts() returns an empty list when getting invalid product', (done) => {
    inAppPurchase.getProducts(['non-exist'], (products) => {
      assert.ok(products.length === 0)
      done()
    })
  })
})
