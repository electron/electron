# BaseView

A `BaseView` is a rectangle within the views View hierarchy. It is the base
class for [`ContainerView`](container-view.md), [`ScrollView`](scroll-view.md).

## Class: BaseView

> The base view for all different views.

Process: [Main](../glossary.md#main-process)

`BaseView` is an [EventEmitter][event-emitter].

### `new BaseView()` _Experimental_

Creates the new base view.

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

#### `view.setBackgroundColor(color)` _Experimental_

* `color` string - Color in `#aarrggbb` or `#argb` form. The alpha channel is
  optional.

Change the background color of the view.

#### `view.getParentView()` _Experimental_

Returns `BaseView || null` - The parent view, otherwise returns `null`.

#### `view.getParentWindow()` _Experimental_

Returns `BrowserWindow || null` - The window that the view belongs to, otherwise returns `null`.

Note: The view can belongs  to either a view or a window. 
