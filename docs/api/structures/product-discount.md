# ProductDiscount Object

* `identifier` string (optional) - A string used to uniquely identify a discount offer for a product.
* `type` number - The type of discount offer.
* `price` number - The discount price of the product in the local currency.
* `priceLocale` string - The locale used to format the discount price of the product.
* `paymentMode` number - The payment mode for this product discount. `pay_as_you_go: 0`, `pay_up_front: 1`, `free_trial: 2`.
* `numberOfPeriods` number - An integer that indicates the number of periods the product discount is available.
* `subscriptionPeriod` [ProductSubscriptionPeriod](product-subscription-period.md) - An object that defines the period for the product discount.
