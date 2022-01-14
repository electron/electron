# Transaction Object

* `transactionIdentifier` string - A string that uniquely identifies a successful payment transaction.
* `transactionDate` string - The date the transaction was added to the App Store’s payment queue.
* `originalTransactionIdentifier` string - The identifier of the restored transaction by the App Store.
* `transactionState` string - The transaction state, can be `purchasing`, `purchased`, `failed`, `restored` or `deferred`.
* `errorCode` Integer - The error code if an error occurred while processing the transaction.
* `errorMessage` string - The error message if an error occurred while processing the transaction.
* `payment` Object
  * `productIdentifier` string - The identifier of the purchased product.
  * `quantity` Integer  - The quantity purchased.
  * `applicationUsername` string - An opaque identifier for the user’s account on your system.
  * `paymentDiscount` [PaymentDiscount](payment-discount.md) (optional) - The details of the discount offer to apply to the payment.
