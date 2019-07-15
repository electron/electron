import { expect } from 'chai'
import { inAppPurchase } from 'electron'

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return

  this.timeout(3 * 60 * 1000)

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
    expect(inAppPurchase.getReceiptURL()).to.match(/_MASReceipt\/receipt$/)
  })

  // The following three tests are disabled because they hit Apple servers, and
  // Apple started blocking requests from AWS IPs (we think), so they fail on
  // CI.
  // TODO: find a way to mock out the server requests so we can test these APIs
  // without relying on a remote service.
  xit('purchaseProduct() fails when buying invalid product', async () => {
    const success = await inAppPurchase.purchaseProduct('non-exist', 1)
    expect(success).to.be.false('failed to purchase non-existent product')
  })

  xit('purchaseProduct() accepts optional arguments', async () => {
    const success = await inAppPurchase.purchaseProduct('non-exist')
    expect(success).to.be.false('failed to purchase non-existent product')
  })

  xit('getProducts() returns an empty list when getting invalid product', async () => {
    const products = await inAppPurchase.getProducts(['non-exist'])
    expect(products).to.be.an('array').of.length(0)
  })
})
