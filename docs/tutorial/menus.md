---
title: Menus
description: Configure cross-platform native OS menus with the Menu API.
slug: menus
hide_title: true
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';
import DocCardList from '@theme/DocCardList';

# Menus

Electron's [Menu](../api/menu.md) class provides a standardized way to create cross-platform native
menus throughout your application.

## Available menus in Electron

The same menu API is used for multiple use cases:

* The **application menu** is the top-level menu for your application. Each app only has a single
  application menu at a time.
* **Context menus** are triggered by the user when right-clicking on a portion of your app's
  interface.
* The **tray menu** is a special context menu triggered when right-clicking on your app's [Tray](../api/tray.md)
  instance.
* On macOS, the **dock menu** is a special context menu triggered when right-clicking on your app's
  icon in the system [Dock](https://support.apple.com/en-ca/guide/mac-help/mh35859/mac).

To learn more about the various kinds of native menus you can create and how to specify keyboard
shortcuts, see the individual guides in this section:

<DocCardList />

## Building menus

Each `Menu` instance is composed of an array of [MenuItem](../api/menu-item.md) objects accessible via
the `menu.items` instance property. Menus can be nested by setting the `item.submenu` property to
another menu.

There are two ways to build a menu: either by directly calling [`menu.append`](../api/menu.md#menuappendmenuitem)
or by using the static [`Menu.buildFromTemplate`](../api/menu.md#menubuildfromtemplatetemplate)
helper function.

The helper function reduces boilerplate by allowing you to pass a collection of `MenuItem`
constructor options (or instantiated `MenuItem` instances) in a single array rather than having to
append each item in a separate function call.

Below is an example of a minimal application menu:

<Tabs>
  <TabItem value="constructor" label="Constructor">
    ```js title='menu.js'
    const submenu = new Menu()
    submenu.append(new MenuItem({ label: 'Hello' }))
    submenu.append(new MenuItem({ type: 'separator' }))
    submenu.append(new MenuItem({ label: 'Electron', type: 'checkbox', checked: true }))
    const menu = new Menu()
    menu.append(new MenuItem({ label: 'Menu', submenu }))
    Menu.setApplicationMenu(menu)
    ```
  </TabItem>
  <TabItem value="template" label="Template Helper">
    ```js title='menu.js'
    const menu = Menu.buildFromTemplate([{
      label: 'Menu',
      submenu: [
        { label: 'Hello' },
        { type: 'separator' },
        { label: 'Electron', type: 'checkbox', checked: true }
      ]
    }])
    Menu.setApplicationMenu(menu)
    ```
  </TabItem>
</Tabs>

> [!IMPORTANT]
> All menu items (except for the `separator` type) must have a label. Labels can either be manually
> defined using the `label` property or inherited from the item's `role`.

### Types

A menu item's type grants it a particular appearance and functionality. Some types are
automatically assigned based on other constructor options:

* By default, menu items have the `normal` type.
* Menu items that contain the `submenu` property will be assigned the `submenu` type.

Other available types, when specified, give special additional properties to the menu item:

* `checkbox` - toggles the `checked` property whenever the menu item is clicked
* `radio` - toggles the `checked` property and turns off that property for all adjacent `radio` items
* `palette` - creates a [Palette](https://developer.apple.com/documentation/appkit/nsmenu/presentationstyle-swift.enum/palette)
submenu, which aligns items horizontally (available on macOS 14 and above)
* `header` - creates a section header, which can convey groupings with labels (available on macOS 14 and above)

> [!TIP]
> Adjacent `radio` items are at the same level of submenu and not divided by a separator.
>
> ```js
> [
>   { type: 'radio', label: 'Adjacent 1' },
>   { type: 'radio', label: 'Adjacent 2' },
>   { type: 'separator' },
>   { type: 'radio', label: 'Non-adjacent' } // unaffected by the others
> ]
> ```

### Roles

Roles give `normal` type menu items predefined behaviors.

We recommend specifying the `role` attribute for any menu item that matches a standard role
rather than trying to manually implement the behavior in a `click` function.
The built-in `role` behavior will give the best native experience.

The `label` and `accelerator` values are optional when using a `role` and will
default to appropriate values for each platform.

> [!TIP]
> Role strings are **case-insensitive**. For example, `toggleDevTools`, `toggledevtools`, and
> `TOGGLEDEVTOOLS` are all equivalent roles when defining menu items.

#### Edit roles

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `pasteAndMatchStyle`
* `selectAll`
* `delete`

#### Window roles

* `about` - Trigger a native about panel (custom message box on Window, which does not provide its own).
* `minimize` - Minimize current window.
* `close` - Close current window.
* `quit` - Quit the application.
* `reload` - Reload the current window.
* `forceReload` - Reload the current window ignoring the cache.
* `toggleDevTools` - Toggle developer tools in the current window.
* `togglefullscreen` - Toggle full screen mode on the current window.
* `resetZoom` - Reset the focused page's zoom level to the original size.
* `zoomIn` - Zoom in the focused page by 10%.
* `zoomOut` - Zoom out the focused page by 10%.
* `toggleSpellChecker` - Enable/disable built-in spellchecker.

#### Default menu roles

* `fileMenu` - The submenu is a "File" menu (Close / Quit)
* `editMenu` - The submenu is an "Edit" menu (Undo, Copy, etc.)
* `viewMenu` - The submenu is a "View" menu (Reload, Toggle Developer Tools, etc.)
* `windowMenu` - The submenu is a "Window" menu (Minimize, Zoom, etc.)

#### macOS-only roles

macOS has a number of platform-specific menu roles available. Many of these map to underlying
[AppKit](https://developer.apple.com/documentation/appkit) APIs.

##### App management roles

* `hide` - Map to the [`hide`](https://developer.apple.com/documentation/appkit/nsapplication/hide(_:)) action.
* `hideOthers` - Map to the [`hideOtherApplications`](https://developer.apple.com/documentation/appkit/nsapplication/hideotherapplications(_:)) action.
* `unhide` - Map to the [`unhideAllApplications`](https://developer.apple.com/documentation/appkit/nsapplication/unhideallapplications(_:)) action.
* `front` - Map to the [`arrangeInFront`](https://developer.apple.com/documentation/appkit/nsapplication/arrangeinfront(_:)) action.
* `zoom` - Map to the [`performZoom`](https://developer.apple.com/documentation/appkit/nswindow/performzoom(_:)) action.

##### Edit roles

* `showSubstitutions` - Map to the [`orderFrontSubstitutionsPanel`](https://developer.apple.com/documentation/appkit/nstextview/orderfrontsubstitutionspanel(_:)) action.
* `toggleSmartQuotes` - Map to the [`toggleAutomaticQuoteSubstitution`](https://developer.apple.com/documentation/appkit/nstextview/toggleautomaticquotesubstitution(_:)) action.
* `toggleSmartDashes` - Map to the [`toggleAutomaticDashSubstitution`](https://developer.apple.com/documentation/appkit/nstextview/toggleautomaticdashsubstitution(_:)) action.
* `toggleTextReplacement` - Map to the [`toggleAutomaticTextReplacement`](https://developer.apple.com/documentation/appkit/nstextview/toggleautomatictextreplacement(_:)) action.

##### Speech roles

* `startSpeaking` - Map to the [`startSpeaking`](https://developer.apple.com/documentation/appkit/nstextview/startspeaking(_:)) action.
* `stopSpeaking` - Map to the [`stopSpeaking`](https://developer.apple.com/documentation/appkit/nstextview/stopspeaking(_:)) action.

##### Native tab roles

* `toggleTabBar` - Map to the [`toggleTabBar`](https://developer.apple.com/documentation/appkit/nswindow/toggletabbar(_:)) action.
* `selectNextTab` - Map to the [`selectNextTab`](https://developer.apple.com/documentation/appkit/nswindow/selectnexttab(_:)) action.
* `selectPreviousTab` - Map to the [`selectPreviousTab`](https://developer.apple.com/documentation/appkit/nswindow/selectprevioustab(_:)) action.
<!-- * `showAllTabs` - Map to the `showAllTabs` action. -->
* `mergeAllWindows` - Map to the [`mergeAllWindows`](https://developer.apple.com/documentation/appkit/nswindow/mergeallwindows(_:)) action.
* `moveTabToNewWindow` - Map to the [`moveTabToNewWindow`](https://developer.apple.com/documentation/appkit/nswindow/movetabtonewwindow(_:)) action.

##### Default menu roles

* `appMenu` - Whole default "App" menu (About, Services, etc.)
* `services` - The submenu is a ["Services"](https://developer.apple.com/documentation/appkit/nsapplication/1428608-servicesmenu?language=objc) menu.
* `window` - The submenu is a "Window" menu.
* `help` - The submenu is a "Help" menu.

##### Other menu roles

* `recentDocuments` - The submenu is an "Open Recent" menu.
* `clearRecentDocuments` - Map to the [`clearRecentDocuments`](https://developer.apple.com/documentation/appkit/nsdocumentcontroller/clearrecentdocuments(_:)) action.
* `shareMenu` - The submenu is [share menu](../api/share-menu.md). The `sharingItem` property must also be set to indicate the item to share.

> [!IMPORTANT]
> When specifying a `role` on macOS, `label` and `accelerator` are the only
> options that will affect the menu item. All other options will be ignored.

### Accelerators

The `accelerator` property allows you to define accelerator strings to map menu items to keyboard
shortcuts. For more details, see the [Keyboard Shortcuts](./keyboard-shortcuts.md) guide.

## Advanced configuration

### Programmatic item positioning

You can make use of the `before`, `after`, `beforeGroupContaining`, `afterGroupContaining` and `id` attributes
to control how menu items will be placed when building a menu with `Menu.buildFromTemplate`.

* `before` - Inserts this item before the item with the specified id. If the
  referenced item doesn't exist, the item will be inserted at the end of
  the menu. Also implies that the menu item in question should be placed in the same “group” as the item.
* `after` - Inserts this item after the item with the specified id. If the
  referenced item doesn't exist, the item will be inserted at the end of
  the menu. Also implies that the menu item in question should be placed in the same “group” as the item.
* `beforeGroupContaining` - Provides a means for a single context menu to declare
  the placement of their containing group before the containing group of the item with the specified id.
* `afterGroupContaining` - Provides a means for a single context menu to declare
  the placement of their containing group after the containing group of the item with the specified id.

By default, items will be inserted in the order they exist in the template unless one of the specified
positioning keywords is used.

#### Examples

Template:

```js
[
  { id: '1', label: 'one' },
  { id: '2', label: 'two' },
  { id: '3', label: 'three' },
  { id: '4', label: 'four' }
]
```

Menu:

```plaintext
- one
- two
- three
- four
```

Template:

```js
[
  { id: '1', label: 'one' },
  { type: 'separator' },
  { id: '3', label: 'three', beforeGroupContaining: ['1'] },
  { id: '4', label: 'four', afterGroupContaining: ['2'] },
  { type: 'separator' },
  { id: '2', label: 'two' }
]
```

Menu:

```plaintext
- three
- four
- ---
- one
- ---
- two
```

Template:

```js
[
  { id: '1', label: 'one', after: ['3'] },
  { id: '2', label: 'two', before: ['1'] },
  { id: '3', label: 'three' }
]
```

Menu:

```plaintext
- ---
- three
- two
- one
```

### Icons

To add visual aid to your menus, you can use the `icon` property to assign images to individual
`MenuItem` instances.

```js title='Adding a little green circle to a menu item'
const { nativeImage } = require('electron/common')
const { MenuItem } = require('electron/main')

// highlight-next-line
const green = nativeImage.createFromDataURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAACOSURBVHgBpZLRDYAgEEOrEzgCozCCGzkCbKArOIlugJvgoRAUNcLRpvGH19TkgFQWkqIohhK8UEaKwKcsOg/+WR1vX+AlA74u6q4FqgCOSzwsGHCwbKliAF89Cv89tWmOT4VaVMoVbOBrdQUz+FrD6XItzh4LzYB1HFJ9yrEkZ4l+wvcid9pTssh4UKbPd+4vED2Nd54iAAAAAElFTkSuQmCC')

const item = new MenuItem({
  label: 'Green Circle',
  // highlight-next-line
  icon: green
})
```

### Sublabels _macOS_

You can add sublabels (also known as [subtitles](https://developer.apple.com/documentation/appkit/nsmenuitem/subtitle))
to menu items using the `sublabel` option on macOS 14.4 and above.

```js title='Adding descriptions via sublabel'
const { MenuItem } = require('electron/main')

const item = new MenuItem({
  label: 'Log Message',
  // highlight-next-line
  sublabel: 'This will use the console.log utility',
  click: () => { console.log('Logging via menu...') }
})
```

### Tooltips _macOS_

Tooltips are informational indicators that appear when you hover over a menu item. You can set menu
item tooltips on macOS using the `toolTip` option.

```js title='Adding additional information via tooltip'
const { MenuItem } = require('electron/main')

const item = new MenuItem({
  label: 'Hover Over Me',
  // highlight-next-line
  toolTip: 'This is additional info that appears on hover'
})
```
