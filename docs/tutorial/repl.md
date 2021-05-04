# REPL

[Read-Eval-Print-Loop](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop) (REPL)
is a simple, interactive computer programming environment that takes single user
inputs (i.e. single expressions), evaluates them, and returns the result to the user.

## Main process

Electron exposes the [Node.js `repl` module](https://nodejs.org/dist/latest/docs/api/repl.html)
through the `--interactive` CLI flag. Assuming you have `electron` installed as a local project
dependency, you should be able to access the REPL with the following command:

  ```sh
  ./node_modules/.bin/electron --interactive
  ```

**Note:** `electron --interactive` is not available on Windows
(see [electron/electron#5776](https://github.com/electron/electron/pull/5776) for more details).

## Renderer process

You can use the DevTools Console tab to get a REPL for any renderer process.
To learn more, read [the Chrome documentation](https://developer.chrome.com/docs/devtools/console/).
