'use strict'

const binding = process.atomBinding('in_app_purchase')
const v8Util = process.atomBinding('v8_util')

module.exports = {

  canMakePayments: function () {
    return binding.canMakePayments()
  },

  getReceiptURL: function () {
    return binding.getReceiptURL()
  },

  purchaseProduct: function (productID, quantity = 1, callback) {
    if (typeof productID !== 'string') {
      throw new TypeError('productID must be a string')
    }
    if (typeof quantity === 'function') {
      quantity = 1
      callback = quantity
    }
    return binding.purchaseProduct(productID, quantity, callback)
  },

  addTransactionListener: function (callback) {
    if (typeof callback !== 'function') {
      throw new TypeError('callback must be a function')
    }
    return binding.addTransactionListener(callback)
  }
}

// Mark standard asynchronous functions.
v8Util.setHiddenValue(
  module.exports.purchaseProduct, 'asynchronous', true)
v8Util.setHiddenValue(
  module.exports.addTransactionListener, 'asynchronous', true)
