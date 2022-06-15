# JumpListCategory Object

* `type` string (optional) - One of the following:
  * `tasks` - Items in this category will be placed into the standard `Tasks`
    category. There can be only one such category, and it will always be
    displayed at the bottom of the Jump List.
  * `frequent` - Displays a list of files frequently opened by the app, the
    name of the category and its items are set by Windows.
  * `recent` - Displays a list of files recently opened by the app, the name
    of the category and its items are set by Windows. Items may be added to
    this category indirectly using `app.addRecentDocument(path)`.
  * `custom` - Displays tasks or file links, `name` must be set by the app.
* `name` string (optional) - Must be set if `type` is `custom`, otherwise it should be
  omitted.
* `items` JumpListItem[] (optional) - Array of [`JumpListItem`](jump-list-item.md) objects if `type` is `tasks` or
  `custom`, otherwise it should be omitted.

**Note:** If a `JumpListCategory` object has neither the `type` nor the `name`
property set then its `type` is assumed to be `tasks`. If the `name` property
is set but the `type` property is omitted then the `type` is assumed to be
`custom`.

**Note:** The maximum length of a Jump List item's `description` property is
260 characters. Beyond this limit, the item will not be added to the Jump
List, nor will it be displayed.
