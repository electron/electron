const { app, WebContentsView } = require('electron');

const v8 = require('node:v8');

// Force V8 to schedule incremental-marking finalization steps as foreground
// tasks on every allocation. Those tasks run inside V8's
// DisallowJavascriptExecutionScope. Prior to the fix in
// shell/common/gin_helper/wrappable.cc, gin_helper::SecondWeakCallback would
// `delete` the Wrappable synchronously inside that scope, and Wrappables
// whose destructors emit JS events (WebContents emits 'will-destroy' /
// 'destroyed') would crash with "Invoke in DisallowJavascriptExecutionScope".
//
// Regression test for https://github.com/electron/electron/issues/47420.
v8.setFlagsFromString('--stress-incremental-marking');

app.whenReady().then(() => {
  // Leak several WebContentsView instances so at least one wrapper's
  // collection lands in a foreground GC task during app.quit()'s uv_run
  // drain. Three is enough for the crash to reproduce 10/10 on a Linux
  // testing build before the fix.
  // eslint-disable-next-line no-new
  new WebContentsView();
  // eslint-disable-next-line no-new
  new WebContentsView();
  // eslint-disable-next-line no-new
  new WebContentsView();

  app.quit();
});
