'use strict'

const { deprecate } = require('electron')
const { markPromisified } = require('@electron/internal/common/promise-utils')

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

// Mark promisifed APIs
markPromisified(module.exports.purchaseProduct)
markPromisified(module.exports.getProducts)
