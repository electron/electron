# menu-item

## Class: MenuItem

### new MenuItem(options)

* `options` Object

  * `click` Function - Callback when the menu item is clicked

  * `selector` String - Call the selector of first responder when clicked (OS
     X only)

  * `type` String - Can be `normal`, `separator`, `submenu`, `checkbox` or
     `radio`

  * `label` String

  * `sublabel` String

  * `accelerator` [Accelerator](accelerator.md)

  * `icon` [NativeImage](native-image.md)

  * `enabled` Boolean

  * `visible` Boolean

  * `checked` Boolean

  * `submenu` Menu - Should be specified for `submenu` type menu item, when
     it's specified the `type: 'submenu'` can be omitted for the menu item

  * `id` String - Unique within a single menu. If defined then it can be used
     as a reference to this item by the position attribute.

  * `position` String - This field allows fine-grained definition of the specific
     location within a given menu.

     It has the form "[placement]=[id]" where placement is one of `before`,
     `after`, or `endof` and `id` is the id of an existing item in the menu.

     - `before` - Inserts this item before the id referenced item. If the
        referenced item doesn't exist the item will be inserted at the end of
        the menu.

     - `after` - Inserts this item after id referenced item. If the referenced
        item doesn't exist the item will be inserted at the end of the menu.

     - `endof` - Inserts this item at the end of the logical group containing
        the id referenced item. (Groups are created by separator items). If
        the referenced item doesn't exist a new separator group is created with
        the given id and this item is inserted after that separator.

    When an item is positioned following unpositioned items are inserted after
    it, until a new item is positioned. So if you want to position a group of
    menu items in the same location you only need to specify a position for
    the first item.