# Transaction Object

* `transactionIdentifier` String - A string that uniquely identifies a successful payment transaction.
* `transactionDate` String - The date the transaction was added to the App Store’s payment queue.
* `originalTransactionIdentifier` String - The identifier of the restored transaction by the App Store.
* `transactionState` String - The transaction state, can be `purchasing`, `purchased`, `failed`, `restored` or `deferred`.
* `errorCode` Integer - The error code if an error occurred while processing the transaction.
* `errorMessage` String - The error message if an error occurred while processing the transaction.
* `payment` Object
  * `productIdentifier` String - The identifier of the purchased product.
  * `quantity` Integer  - The quantity purchased.
