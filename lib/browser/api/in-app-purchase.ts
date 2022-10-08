import { EventEmitter } from 'events';

let _inAppPurchase;

if (process.platform === 'darwin') {
  const { inAppPurchase } = process._linkedBinding('electron_browser_in_app_purchase');

  _inAppPurchase = inAppPurchase;
  _inAppPurchase.purchaseProduct = (productID: string, opts?: number | { quantity?: number, username?: string }) => {
    const quantity = typeof opts === 'object' ? opts.quantity : opts;
    const username = typeof opts === 'object' ? opts.username : undefined;
    return inAppPurchase.purchaseProduct(productID, quantity, username);
  };
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
