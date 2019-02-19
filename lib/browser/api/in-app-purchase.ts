import { EventEmitter } from 'events'

class UnsupportedInAppPurchaseAPI extends EventEmitter implements Electron.InAppPurchase {
  purchaseProduct (productID: string, quantity?: number, callback?: (isProductValid: boolean) => void) {
    throw new Error('The inAppPurchase module can only be used on macOS')
  }
  canMakePayments () {
    return false
  }
  getReceiptURL () {
    return ''
  }
  finishAllTransactions () {}
  finishTransactionByDate (date: string) {}
  getProducts (productIDs: string[], callback: (products: Electron.Product[]) => void) {
    throw new Error('The inAppPurchase module can only be used on macOS')
  }
}

let inAppPurchaseImpl: Electron.InAppPurchase = new UnsupportedInAppPurchaseAPI()

if (process.platform === 'darwin') {
  const { inAppPurchase, InAppPurchase } = process.atomBinding('in_app_purchase')

  // inAppPurchase is an EventEmitter.
  Object.setPrototypeOf(InAppPurchase.prototype, EventEmitter.prototype)
  EventEmitter.call(inAppPurchase as any)

  inAppPurchaseImpl = inAppPurchase
}

export default inAppPurchaseImpl
