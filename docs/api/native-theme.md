# nativeTheme

> Read and respond to changes in Chromium's native color theme.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Events

The `nativeTheme` module emits the following events:

### Event: 'updated'

Emitted when something in the underlying NativeTheme has changed. This normally
means that either the value of `shouldUseDarkColors`,
`shouldUseHighContrastColors` or `shouldUseInvertedColorScheme` has changed.
You will have to check them to determine which one has changed.

## Properties

The `nativeTheme` module has the following properties:

### `nativeTheme.shouldUseDarkColors` _Readonly_

A `Boolean` for if the OS / Chromium currently has a dark mode enabled or is
being instructed to show a dark-style UI.

### `nativeTheme.shouldUseHighContrastColors` _macOS_ _Windows_ _Readonly_

A `Boolean` for if the OS / Chromium currently has high-contrast mode enabled
or is being instructed to show a high-constrast UI.

### `nativeTheme.shouldUseInvertedColorScheme` _macOS_ _Windows_ _Readonly_

A `Boolean` for if the OS / Chromium currently has an inverted color scheme
or is being instructed to use an inverted color scheme.
