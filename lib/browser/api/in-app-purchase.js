'use strict'

const { deprecate } = require('electron')

if (process.platform === 'darwin') {
  const { EventEmitter } = require('events')
  const { inAppPurchase, InAppPurchase } = process.electronBinding('in_app_purchase')

  // inAppPurchase is an EventEmitter.
  Object.setPrototypeOf(InAppPurchase.prototype, EventEmitter.prototype)
  EventEmitter.call(inAppPurchase)

  module.exports = inAppPurchase
} else {
  module.exports = {
    purchaseProduct: (productID, quantity, callback) => {
      throw new Error('The inAppPurchase module can only be used on macOS')
    },
    canMakePayments: () => false,
    getReceiptURL: () => ''
  }
}

module.exports.purchaseProduct = deprecate.promisify(module.exports.purchaseProduct)
module.exports.getProducts = deprecate.promisify(module.exports.getProducts)
