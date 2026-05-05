# MenuItemBadge Object

* `type` string (optional) - Can be `alerts`, `updates`, `new-items` or `none`. Default is `none`.  See https://developer.apple.com/documentation/appkit/nsmenuitembadge#Creating-badges-of-a-specific-type for further explanation of these types.
* `count` number (optional) - The number of items the badge displays. Cannot be used with `type: 'none'`.
* `content` string (optional) - A custom string to display in the badge. Only usable with `type: 'none'`.
