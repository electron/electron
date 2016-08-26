# REPL

The `repl` provides a Read-Eval-Print-Loop (REPL) implementation, also known as an interactive toplevel. It is a simple, interactive computer programming environment that takes single user inputs (i.e. single expressions), evaluates them, and returns the result to the user.

## Launching RELP

Assuming you have electron or electron-prebuilt installed as a local project dependency:
`./node_modules/.bin/electron --interactive`

Assuming you have electron or electron-prebuilt installed globally:
`electron --interactive`

### Commands and Special Keys

The following special commands are supported by all REPL instances:

* `.break` - When in the process of inputting a multi-line expression, entering
  the `.break` command (or pressing the `<ctrl>-C` key combination) will abort
  further input or processing of that expression.
* `.clear` - Resets the REPL `context` to an empty object and clears any
  multi-line expression currently being input.
* `.exit` - Close the I/O stream, causing the REPL to exit.
* `.help` - Show this list of special commands.
* `.save` - Save the current REPL session to a file:
  `> .save ./file/to/save.js`
* `.load` - Load a file into the current REPL session.
  `> .load ./file/to/load.js`
* `.editor` - Enter editor mode (`<ctrl>-D` to finish, `<ctrl>-C` to cancel)


The following key combinations in the REPL have these special effects:

* `<ctrl>-C` - When pressed once, has the same effect as the `.break` command.
  When pressed twice on a blank line, has the same effect as the `.exit`
  command.
* `<ctrl>-D` - Has the same effect as the `.exit` command.
* `<tab>` - When pressed on a blank line, displays global and local(scope)
  variables. When pressed while entering other input, displays relevant
  autocompletion options.


#### JavaScript Expressions

The default evaluator supports direct evaluation of JavaScript expressions:

```js
> 1 + 1
2
> var m = 2
undefined
> m + 1
3
```

Unless otherwise scoped within blocks (e.g. `{ ... }`) or functions, variables
declared either implicitly or using the `var` keyword are declared at the
`global` scope.



### Event: 'exit'
<!-- YAML
added: v0.7.7
-->

The `'exit'` event is emitted when the REPL is exited either by receiving the
`.exit` command as input, the user pressing `<ctrl>-C` twice to signal `SIGINT`,
or by pressing `<ctrl>-D` to signal `'end'` on the input stream. The listener
callback is invoked without any arguments.

```js
replServer.on('exit', () => {
  console.log('Received "exit" event from repl!');
  process.exit();
});
```

### Event: 'reset'
<!-- YAML
added: v0.11.0
-->

The `'reset'` event is emitted when the REPL's context is reset. This occurs
whenever the `.clear` command is received as input.

## The Node.js REPL

Node.js itself uses the `repl` module to provide its own interactive interface
for executing JavaScript. This can used by executing the Node.js binary without
passing any arguments (or by passing the `-i` argument):

```js
$ node
> a = [1, 2, 3];
[ 1, 2, 3 ]
> a.forEach((v) => {
...   console.log(v);
...   });
1
2
3
```

Read more Node.js `REPL` documentation at https://github.com/nodejs/node/blob/master/doc/api/repl.md
