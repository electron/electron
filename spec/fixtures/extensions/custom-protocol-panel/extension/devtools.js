/* global chrome */
chrome.devtools.panels.create('ELECTRON TEST PANEL', '', 'panel.html');

// The fixture's main process can only attach its `console-message` listener
// after `devtools-opened`, which the browser emits AFTER injecting this
// devtools_page (see InspectableWebContents::LoadCompleted: AddDevToolsExtensionsToClient
// runs before DevToolsOpened/`devtools-opened`). A single synchronous log can
// therefore fire before the listener exists (a lost-event race, observed as
// empty stdout under ASan). Re-announce until the parent receives it and quits.
const announce = () => console.log('ELECTRON TEST PANEL created');
announce();
setInterval(announce, 100);
