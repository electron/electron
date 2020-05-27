import { EventEmitter } from 'events';

let _inAppPurchase;

if (process.platform === 'darwin') {
  const { inAppPurchase, InAppPurchase } = process.electronBinding('in_app_purchase');

  // inAppPurchase is an EventEmitter.
  Object.setPrototypeOf(InAppPurchase.prototype, EventEmitter.prototype);
  EventEmitter.call(inAppPurchase);

  _inAppPurchase = inAppPurchase;
} else {
  _inAppPurchase = new EventEmitter();
  Object.assign(_inAppPurchase, {
    purchaseProduct: () => {
      throw new Error('The inAppPurchase module can only be used on macOS');
    },
    canMakePayments: () => false,
    getReceiptURL: () => ''
  });
}

export default _inAppPurchase;
