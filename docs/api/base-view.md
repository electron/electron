# BaseView

A `BaseView` is a rectangle within the views View hierarchy. It is the base
class for [`ContainerView`](container-view.md), [`ScrollView`](scroll-view.md).

## Class: BaseView

> The base view for all different views.

Process: [Main](../glossary.md#main-process)

`BaseView` is an [EventEmitter][event-emitter].

### `new BaseView()` _Experimental_

Creates the new base view.

### Instance Events

Objects created with `new BaseView` emit the following events:

#### Event: 'size-changed' _Experimental_

Returns:

* `event` Event
* `oldSize` [Size](structures/size.md) - Size the view was before.
* `newSize` [Size](structures/size.md) - Size the view is being resized to.

Emitted when the view's size has been changed.

### Static Methods

The `BaseView` class has the following static methods:

#### `BaseView.getAllViews()`

Returns `BaseView[]` - An array of all created views.

#### `BaseView.fromId(id)`

* `id` Integer

Returns `BaseView | null` - The view with the given `id`.

### Instance Properties

Objects created with `new BaseView` have the following properties:

#### `view.id` _Readonly_ _Experimental_

A `Integer` property representing the unique ID of the view. Each ID is unique among all `BaseView` instances of the entire Electron application.

#### `view.isContainer` _Readonly_ _Experimental_

A `boolean` property that determines whether this view is (or inherits from) [`ContainerView`](container-view.md).

### Instance Methods

Objects created with `new BaseView` have the following instance methods:

#### `view.setBounds(bounds)` _Experimental_

* `bounds` [Rectangle](structures/rectangle.md) - The position and size of the view, relative to its parent.

Resizes and moves the view to the supplied bounds relative to its parent.

#### `view.getBounds()` _Experimental_

Returns [`Rectangle`](structures/rectangle.md)

Returns the position and size of the view, relative to its parent.

#### `view.offsetFromView(from)` _Experimental_

* `from` `BaseView`

Returns [`Point`](structures/point.md) - Offset from `from` view.

Converts `(0, 0)` point from the coordinate system of a given `from` to that of the view.
both `from` and the view must belong to the same [`BrowserWindow`](browser-window.md).

#### `view.offsetFromWindow()` _Experimental_

Returns [`Point`](structures/point.md) - Offset the window that owns the view.

Converts `(0, 0)` point from window base coordinates to that of the view.

#### `view.setVisible(visible)` _Experimental_

* `visible` boolean - Sets whether this view is visible.

#### `view.isVisible()` _Experimental_

Returns `boolean` - Whether the view is visible.

#### `view.IsTreeVisible()` _Experimental_

Returns `boolean` - Whether the view and its parent are visible.

#### `view.focus()` _Experimental_

Move the keyboard focus to the view.

#### `view.hasFocus()` _Experimental_

Returns `boolean` - Whether this view currently has the focus.

#### `view.setFocusable(focusable)` _Experimental_

* `focusable` boolean - Sets whether this view can be focused on.

#### `view.isFocusable()` _Experimental_

Returns `boolean` - Returns `true` if this view is focusable, enabled and drawn.

#### `view.setBackgroundColor(color)` _Experimental_

* `color` string - Color in `#aarrggbb` or `#argb` form. The alpha channel is
  optional.

Change the background color of the view.

#### `view.getParentView()` _Experimental_

Returns `BaseView || null` - The parent view, otherwise returns `null`.

#### `view.getParentWindow()` _Experimental_

Returns `BrowserWindow || null` - The window that the view belongs to, otherwise returns `null`.

Note: The view can belongs  to either a view or a window. 
