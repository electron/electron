'use strict'

const assert = require('assert')

const {remote} = require('electron')
const {inAppPurchase} = remote

describe('inAppPurchase module', () => {
  if (process.platform !== 'darwin') return

  it('canMakePayments() does not throw', () => {
    inAppPurchase.canMakePayments()
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
})
