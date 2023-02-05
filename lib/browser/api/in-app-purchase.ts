import { EventEmitter } from 'events';

let _inAppPurchase;

if (process.platform === 'darwin') {
  const { inAppPurchase } = process._linkedBinding('electron_browser_in_app_purchase');
  const _purchase = inAppPurchase.purchaseProduct as (productID: string, quantity?: number, username?: string) => Promise<boolean>;
  inAppPurchase.purchaseProduct = (productID: string, opts?: number | { quantity?: number, username?: string }) => {
    const quantity = typeof opts === 'object' ? opts.quantity : opts;
    const username = typeof opts === 'object' ? opts.username : undefined;
    return _purchase.apply(inAppPurchase, [productID, quantity, username]);
  };
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
