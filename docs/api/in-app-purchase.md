# inAppPurchase

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/11292
```
-->

> In-app purchases on Mac App Store.

Process: [Main](../glossary.md#main-process)

## Events

The `inAppPurchase` module emits the following events:

### Event: 'transactions-updated'

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/11292
```
-->

Returns:

* `event` Event
* `transactions` Transaction[] - Array of [`Transaction`](structures/transaction.md) objects.

Emitted when one or more transactions have been updated.

## Methods

The `inAppPurchase` module has the following methods:

### `inAppPurchase.purchaseProduct(productID[, opts])`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/11292
changes:
  - pr-url: https://github.com/electron/electron/pull/17355
    description: "This method now returns a Promise instead of using a callback function."
    breaking-changes-header: api-changed-callback-based-versions-of-promisified-apis
  - pr-url: https://github.com/electron/electron/pull/35902
    description: "Added `username` option to `opts` parameter."
```
-->

* `productID` string
* `opts` Integer | Object (optional) - If specified as an integer, defines the quantity.
  * `quantity` Integer (optional) - The number of items the user wants to purchase.
  * `username` string (optional) - The string that associates the transaction with a user account on your service (applicationUsername).

Returns `Promise<boolean>` - Returns `true` if the product is valid and added to the payment queue.

You should listen for the `transactions-updated` event as soon as possible and certainly before you call `purchaseProduct`.

### `inAppPurchase.getProducts(productIDs)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/12464
changes:
  - pr-url: https://github.com/electron/electron/pull/17355
    description: "This method now returns a Promise instead of using a callback function."
    breaking-changes-header: api-changed-callback-based-versions-of-promisified-apis
```
-->

* `productIDs` string[] - The identifiers of the products to get.

Returns `Promise<Product[]>` - Resolves with an array of [`Product`](structures/product.md) objects.

Retrieves the product descriptions.

### `inAppPurchase.canMakePayments()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/11292
```
-->

Returns `boolean` - whether a user can make a payment.

### `inAppPurchase.restoreCompletedTransactions()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/21461
```
-->

Restores finished transactions. This method can be called either to install purchases on additional devices, or to restore purchases for an application that the user deleted and reinstalled.

[The payment queue](https://developer.apple.com/documentation/storekit/skpaymentqueue?language=objc) delivers a new transaction for each previously completed transaction that can be restored. Each transaction includes a copy of the original transaction.

### `inAppPurchase.getReceiptURL()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/11292
```
-->

Returns `string` - the path to the receipt.

### `inAppPurchase.finishAllTransactions()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/12464
```
-->

Completes all pending transactions.

### `inAppPurchase.finishTransactionByDate(date)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/12464
```
-->

* `date` string - The ISO formatted date of the transaction to finish.

Completes the pending transactions corresponding to the date.
