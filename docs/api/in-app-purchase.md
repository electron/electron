# inAppPurchase  _macOS_

Your application should add a listener before to purchase a product. If there are no listener attached to the queue, the payment queue does not synchronize its list of pending transactions with the Apple App Store.

To test it in development, you have to set your app bundle identifier in the `/node_modules/electron/dist/Electron.app/Contents/Info.plist`.

Don't forget to configure your products in itunes Connect (see [In-App Purchase Configuration Guide for iTunes Connect](https://developer.apple.com/library/content/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/Introduction.html)).


## Methods 

The `inAppPurchase` module has the following methods:

### `inAppPurchase.addTransactionListener(listener)`

* `listener` Function -  Called when transactions are updated by the payment queue.
  * `payment` Object
    * `productIdentifier` String
    * `quantity` Integer
  * `transaction` Object
    * `transactionIdentifier` String
    * `transactionDate` String
    * `originalTransactionIdentifier` String
    * `transactionState` String - The transaction sate (`"SKPaymentTransactionStatePurchasing"`, `"SKPaymentTransactionStatePurchased"`, `"SKPaymentTransactionStateFailed"`, `"SKPaymentTransactionStateRestored"`, or `"SKPaymentTransactionStateDeferred"`)
    * `errorCode` Integer
    * `errorMessage` String
   

### `inAppPurchase.purchaseProduct(productID, quantity, callback)`
* `productID` String - The id of the product to purchase. (the id of `com.example.app.product1` is `product1`).
* `quantity` Integer (optional) - The number of items the user wants to purchase.
* `callback` Function (optional) - The callback called when the payment is added to the PaymentQueue. (You should add a listener with `inAppPurchase.addTransactionsListener` to get the transaction status).
  * `isProductValid` Boolean - Determine if the product is valid and added to the payment queue.

### `inAppPurchase.canMakePayments()`

Returns `true` if the user can make a payment and `false` otherwise.

### `inAppPurchase.getReceiptURL()`

Returns `String`, the path to the receipt.
