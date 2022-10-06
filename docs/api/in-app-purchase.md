# inAppPurchase

> In-app purchases on Mac App Store.

Process: [Main](../glossary.md#main-process)

## Events

The `inAppPurchase` module emits the following events:

### Event: 'transactions-updated'

Emitted when one or more transactions have been updated.

Returns:

* `event` Event
* `transactions` Transaction[] - Array of [`Transaction`](structures/transaction.md) objects.

## Methods

The `inAppPurchase` module has the following methods:

### `inAppPurchase.purchaseProduct(productID[, opts])`

* `productID` string
* `opts` Integer | Object (optional) - If specified as an integer, defines the quantity.
  * `quantity` Integer (optional) - The number of items the user wants to purchase.
  * `username` string (optional) - The string that associates the transaction with a user account on your service (applicationUsername).

Returns `Promise<boolean>` - Returns `true` if the product is valid and added to the payment queue.

You should listen for the `transactions-updated` event as soon as possible and certainly before you call `purchaseProduct`.

### `inAppPurchase.getProducts(productIDs)`

* `productIDs` string[] - The identifiers of the products to get.

Returns `Promise<Product[]>` - Resolves with an array of [`Product`](structures/product.md) objects.

Retrieves the product descriptions.

### `inAppPurchase.canMakePayments()`

Returns `boolean` - whether a user can make a payment.

### `inAppPurchase.restoreCompletedTransactions()`

Restores finished transactions. This method can be called either to install purchases on additional devices, or to restore purchases for an application that the user deleted and reinstalled.

[The payment queue](https://developer.apple.com/documentation/storekit/skpaymentqueue?language=objc) delivers a new transaction for each previously completed transaction that can be restored. Each transaction includes a copy of the original transaction.

### `inAppPurchase.getReceiptURL()`

Returns `string` - the path to the receipt.

### `inAppPurchase.finishAllTransactions()`

Completes all pending transactions.

### `inAppPurchase.finishTransactionByDate(date)`

* `date` string - The ISO formatted date of the transaction to finish.

Completes the pending transactions corresponding to the date.
