import { EventEmitter } from 'events';

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

// Populate the methods.
for (const name in binding) {
  if (!name.startsWith('_')) { // some methods are manually populated above
    // TODO(felixrieseberg): Once we can type web_frame natives, we could
    // use a neat `keyof` here
    (WebFrame as any).prototype[name] = function (...args: Array<any>) {
      return binding[name](this.context, ...args);
    };
  }
}

// Helper to return WebFrame or null depending on context.
// TODO(zcbenz): Consider returning same WebFrame for the same frame.
function getWebFrame (context: Window) {
  return context ? new WebFrame(context) : null;
}

const _webFrame = new WebFrame(window);

export default _webFrame;
