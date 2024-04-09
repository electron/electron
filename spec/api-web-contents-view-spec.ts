import { closeAllWindows } from './lib/window-helpers';
import { expect } from 'chai';

import { BaseWindow, View, WebContentsView } from 'electron/main';
import { once } from 'node:events';

describe('WebContentsView', () => {
  afterEach(closeAllWindows);

  it('can be instantiated with no arguments', () => {
    // eslint-disable-next-line no-new
    new WebContentsView();
  });

  it('can be instantiated with no webPreferences', () => {
    // eslint-disable-next-line no-new
    new WebContentsView({});
  });

  it('can be used as content view', () => {
    const w = new BaseWindow({ show: false });
    w.setContentView(new WebContentsView());
  });

  it('can be removed after a close', async () => {
    const w = new BaseWindow({ show: false });
    const v = new View();
    const wcv = new WebContentsView();
    w.setContentView(v);
    v.addChildView(wcv);
    await wcv.webContents.loadURL('about:blank');
    const destroyed = once(wcv.webContents, 'destroyed');
    wcv.webContents.executeJavaScript('window.close()');
    await destroyed;
    expect(wcv.webContents.isDestroyed()).to.be.true();
    v.removeChildView(wcv);
  });

  function triggerGCByAllocation () {
    const arr = [];
    for (let i = 0; i < 1000000; i++) {
      arr.push([]);
    }
    return arr;
  }

  it('doesn\'t crash when GCed during allocation', (done) => {
    // eslint-disable-next-line no-new
    new WebContentsView();
    setTimeout(() => {
      // NB. the crash we're testing for is the lack of a current `v8::Context`
      // when emitting an event in WebContents's destructor. V8 is inconsistent
      // about whether or not there's a current context during garbage
      // collection, and it seems that `v8Util.requestGarbageCollectionForTesting`
      // causes a GC in which there _is_ a current context, so the crash isn't
      // triggered. Thus, we force a GC by other means: namely, by allocating a
      // bunch of stuff.
      triggerGCByAllocation();
      done();
    });
  });

  describe('visibilityState', () => {
    it('is initially hidden', async () => {
      const v = new WebContentsView();
      await v.webContents.loadURL('data:text/html,<script>initialVisibility = document.visibilityState</script>');
      expect(await v.webContents.executeJavaScript('initialVisibility')).to.equal('hidden');
    });

    it('becomes visibile when attached', async () => {
      const v = new WebContentsView();
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('hidden');
      const p = v.webContents.executeJavaScript('new Promise(resolve => document.addEventListener("visibilitychange", resolve))');
      // Ensure that the above listener has been registered before we add the
      // view to the window, or else the visibilitychange event might be
      // dispatched before the listener is registered.
      // executeJavaScript calls are sequential so if this one's finished then
      // the previous one must also have been finished :)
      await v.webContents.executeJavaScript('undefined');
      const w = new BaseWindow();
      w.setContentView(v);
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('visible');
    });

    it('is initially visible if load happens after attach', async () => {
      const w = new BaseWindow();
      const v = new WebContentsView();
      w.contentView = v;
      await v.webContents.loadURL('data:text/html,<script>initialVisibility = document.visibilityState</script>');
      expect(await v.webContents.executeJavaScript('initialVisibility')).to.equal('visible');
    });

    it('becomes hidden when parent window is hidden', async () => {
      const w = new BaseWindow();
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('visible');
      const p = v.webContents.executeJavaScript('new Promise(resolve => document.addEventListener("visibilitychange", resolve))');
      // We have to wait until the listener above is fully registered before hiding the window.
      // On Windows, the executeJavaScript and the visibilitychange can happen out of order
      // without this.
      await v.webContents.executeJavaScript('0');
      w.hide();
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('hidden');
    });

    it('becomes visible when parent window is shown', async () => {
      const w = new BaseWindow({ show: false });
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('hidden');
      const p = v.webContents.executeJavaScript('new Promise(resolve => document.addEventListener("visibilitychange", resolve))');
      // We have to wait until the listener above is fully registered before hiding the window.
      // On Windows, the executeJavaScript and the visibilitychange can happen out of order
      // without this.
      await v.webContents.executeJavaScript('0');
      w.show();
      await p;
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('visible');
    });

    it('does not change when view is moved between two visible windows', async () => {
      const w = new BaseWindow();
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      expect(await v.webContents.executeJavaScript('document.visibilityState')).to.equal('visible');

      const p = v.webContents.executeJavaScript('new Promise(resolve => document.addEventListener("visibilitychange", () => resolve(document.visibilityState)))');
      // Ensure the listener has been registered.
      await v.webContents.executeJavaScript('undefined');
      const w2 = new BaseWindow();
      w2.setContentView(v);
      // Wait for the visibility state to settle as "visible".
      // On macOS one visibilitychange event is fired but visibilityState
      // remains "visible". On Win/Linux, two visibilitychange events are
      // fired, a "hidden" and a "visible" one. Reconcile these two models
      // by waiting until at least one event has been fired, and then waiting
      // until the visibility state settles as "visible".
      let visibilityState = await p;
      for (let attempts = 0; visibilityState !== 'visible' && attempts < 10; attempts++) {
        visibilityState = await v.webContents.executeJavaScript('new Promise(resolve => document.visibilityState === "visible" ? resolve("visible") : document.addEventListener("visibilitychange", () => resolve(document.visibilityState)))');
      }
      expect(visibilityState).to.equal('visible');
    });
  });
});
