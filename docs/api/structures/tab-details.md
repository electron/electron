# TabDetails Object

This object is being used to fill `chrome.tabs.Tab` with app-specific details requested by an extension using for example `chrome.tabs.get`.

- `windowId` Number (optional) - The ID of the window that contains the tab.
- `index` Number (optional) - The zero-based index of the tab within its window.
- `groupId` Number (optional) - The ID of the group that the tab belongs to.
- `openerTabId` Number (optional) - The ID of the tab that opened this tab, if any. This property is only present if the opener tab still exists.
- `active` Number (optional) - Whether the tab is active in its window.
- `highlighted` Boolean (optional) - Whether the tab is highlighted.
- `pinned` Boolean (optional) - Whether the tab is pinned.
- `discarded` Boolean (optional) - Whether the tab is discarded.
- `autoDiscardable` Boolean (optional) - Whether the tab can be discarded automatically when resources are low.
- `mutedReason` String (optional) - The reason the tab was muted or unmuted. Not set if the tab's mute state has never been changed.
- `mutedExtensionId` String (optional) - The ID of the extension that changed the muted state. Not set if an extension was not the reason the muted state last changed.
