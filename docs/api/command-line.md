## Class: CommandLine

> Manipulate the command line arguments for your app that Chromium reads

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

The following example shows how to check if the `--disable-gpu` flag is set.

```js
const { app } = require('electron')

app.commandLine.hasSwitch('disable-gpu')
```

For more information on what kinds of flags and switches you can use, check
out the [Command Line Switches](./command-line-switches.md)
document.

### Instance Methods

#### `commandLine.appendSwitch(switch[, value])`

* `switch` string - A command-line switch, without the leading `--`.
* `value` string (optional) - A value for the given switch.

Append a switch (with optional `value`) to Chromium's command line.

> [!NOTE]
> This will not affect `process.argv`. The intended usage of this function is to
> control Chromium's behavior.

```js
const { app } = require('electron')

app.commandLine.appendSwitch('remote-debugging-port', '8315')
```

#### `commandLine.appendArgument(value)`

* `value` string - The argument to append to the command line.

Append an argument to Chromium's command line. The argument will be quoted
correctly. Switches will precede arguments regardless of appending order.

If you're appending an argument like `--switch=value`, consider using `appendSwitch('switch', 'value')` instead.

```js
const { app } = require('electron')

app.commandLine.appendArgument('--enable-experimental-web-platform-features')
```

> [!NOTE]
> This will not affect `process.argv`. The intended usage of this function is to
> control Chromium's behavior.

#### `commandLine.hasSwitch(switch)`

* `switch` string - A command-line switch.

Returns `boolean` - Whether the command-line switch is present.

```js
const { app } = require('electron')

app.commandLine.appendSwitch('remote-debugging-port', '8315')
const hasPort = app.commandLine.hasSwitch('remote-debugging-port')
console.log(hasPort) // true
```

#### `commandLine.getSwitchValue(switch)`

* `switch` string - A command-line switch.

Returns `string` - The command-line switch value.

This function is meant to obtain Chromium command line switches. It is not
meant to be used for application-specific command line arguments. For the
latter, please use `process.argv`.

```js
const { app } = require('electron')

app.commandLine.appendSwitch('remote-debugging-port', '8315')
const portValue = app.commandLine.getSwitchValue('remote-debugging-port')
console.log(portValue) // '8315'
```

> [!NOTE]
> When the switch is not present or has no value, it returns empty string.

#### `commandLine.removeSwitch(switch)`

* `switch` string - A command-line switch.

Removes the specified switch from Chromium's command line.

```js
const { app } = require('electron')

app.commandLine.appendSwitch('remote-debugging-port', '8315')
console.log(app.commandLine.hasSwitch('remote-debugging-port')) // true

app.commandLine.removeSwitch('remote-debugging-port')
console.log(app.commandLine.hasSwitch('remote-debugging-port')) // false
```

> [!NOTE]
> This will not affect `process.argv`. The intended usage of this function is to
> control Chromium's behavior.
