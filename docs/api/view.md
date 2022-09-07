# View

> A View.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

```javascript
// TODO example
```

## Class: View

> A View.

Process: [Main](../glossary.md#main-process)

`View` is an [EventEmitter][event-emitter].

### `new View()`

Creates a new `View`.

### Instance Methods

Objects created with `new View` have the following instance methods:

#### `view.addChildView(view)`

* `view` View - child view to add.

#### `view.setBounds(bounds)`

* `bounds` [Rectangle](structures/rectangle.md) - new bounds of the view.

### Instance Properties

Objects created with `new View` have the following properties:

#### `view.children` _Readonly_

A `View[]` property representing the child views of this view.
