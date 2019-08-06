# nativeTheme

> Read and respond to changes in Chromium's native color theme

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Events

The `nativeTheme` module emits the following events:

### Event: 'updated'

Emitted when something in the underlying NativeTheme has changed. This normally
means that either the value of `shouldUseDarkColors` or
`shouldUseHighContrastColors` has changed.  You will have to check both to
determine which one has changed.

## Properties

The `nativeTheme` module has the following properties.

### `nativeTheme.shouldUseDarkColors` _Readonly_

A `Boolean` for if the OS / Chromium currently has a dark mode enabled or is
being instructed to show a dark-style UI.

### `nativeImage.shouldUseHighContrastColors` _Readonly_

A `Boolean` for if the OS / Chromium currently has high-contrast mode enabled
or is being instructed to show a high-constrast UI.
