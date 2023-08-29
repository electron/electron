# Product Object

* `productIdentifier` string - The string that identifies the product to the Apple App Store.
* `localizedDescription` string - A description of the product.
* `localizedTitle` string - The name of the product.
* `price` number - The cost of the product in the local currency.
* `formattedPrice` string - The locale formatted price of the product.
* `currencyCode` string - 3 character code presenting a product's currency based on the ISO 4217 standard.
* `introductoryPrice` [ProductDiscount](product-discount.md) (optional) - The object containing introductory price information for the product.
available for the product.
* `discounts` [ProductDiscount](product-discount.md)[] - An array of discount offers
* `subscriptionGroupIdentifier` string - The identifier of the subscription group to which the subscription belongs.
* `subscriptionPeriod` [ProductSubscriptionPeriod](product-subscription-period.md) (optional) - The period details for products that are subscriptions.
* `isDownloadable` boolean - A boolean value that indicates whether the App Store has downloadable content for this product. `true` if at least one file has been associated with the product.
* `downloadContentVersion` string - A string that identifies the version of the content.
* `downloadContentLengths` number[] - The total size of the content, in bytes.
