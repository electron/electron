import { EventEmitter } from 'events';
import deprecate from '@electron/internal/common/api/deprecate';

const binding = process._linkedBinding('electron_renderer_web_frame');

class WebFrame extends EventEmitter {
  constructor (public context: Window) {
    super();

    // Lots of webview would subscribe to webFrame's events.
    this.setMaxListeners(0);
  }

  findFrameByRoutingId (...args: Array<any>) {
    return getWebFrame(binding._findFrameByRoutingId(this.context, ...args));
  }

  getFrameForSelector (...args: Array<any>) {
    return getWebFrame(binding._getFrameForSelector(this.context, ...args));
  }

  findFrameByName (...args: Array<any>) {
    return getWebFrame(binding._findFrameByName(this.context, ...args));
  }

  get opener () {
    return getWebFrame(binding._getOpener(this.context));
  }

  get parent () {
    return getWebFrame(binding._getParent(this.context));
  }

  get top () {
    return getWebFrame(binding._getTop(this.context));
  }

  get firstChild () {
    return getWebFrame(binding._getFirstChild(this.context));
  }

  get nextSibling () {
    return getWebFrame(binding._getNextSibling(this.context));
  }

  get routingId () {
    return binding._getRoutingId(this.context);
  }
}

const { hasSwitch } = process._linkedBinding('electron_common_command_line');
const worldSafeJS = hasSwitch('world-safe-execute-javascript') || !hasSwitch('context-isolation');

// Populate the methods.
for (const name in binding) {
  if (!name.startsWith('_')) { // some methods are manually populated above
    // TODO(felixrieseberg): Once we can type web_frame natives, we could
    // use a neat `keyof` here
    (WebFrame as any).prototype[name] = function (...args: Array<any>) {
      if (!worldSafeJS && name.startsWith('executeJavaScript')) {
        deprecate.log(`Security Warning: webFrame.${name} was called without worldSafeExecuteJavaScript enabled. This is considered unsafe. worldSafeExecuteJavaScript will be enabled by default in Electron 12.`);
      }
      return binding[name](this.context, ...args);
    };
    // TODO(MarshallOfSound): Remove once the above deprecation is removed
    if (name.startsWith('executeJavaScript')) {
      (WebFrame as any).prototype[`_${name}`] = function (...args: Array<any>) {
        return binding[name](this.context, ...args);
      };
    }
  }
}

// Helper to return WebFrame or null depending on context.
// TODO(zcbenz): Consider returning same WebFrame for the same frame.
function getWebFrame (context: Window) {
  return context ? new WebFrame(context) : null;
}

const _webFrame = new WebFrame(window);

export default _webFrame;
