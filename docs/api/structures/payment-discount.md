# PaymentDiscount Object

* `identifier` string - A string used to uniquely identify a discount offer for a product.
* `keyIdentifier` string - A string that identifies the key used to generate the signature.
* `nonce` string - A universally unique ID (UUID) value that you define.
* `signature` string - A UTF-8 string representing the properties of a specific discount offer, cryptographically signed.
* `timestamp` number - The date and time of the signature's creation in milliseconds, formatted in Unix epoch time.
