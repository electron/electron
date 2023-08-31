import { closeAllWindows } from './lib/window-helpers';
import { expect } from 'chai';

import { BaseWindow, WebContentsView } from 'electron/main';

describe('WebContentsView', () => {
  afterEach(closeAllWindows);
  // let w: BaseWindow;

  // afterEach(async () => {
  //   await closeWindow(w as any);
  //   w = null as unknown as BaseWindow;
  // });

  it('can be used as content view', () => {
    const w = new BaseWindow({ show: false });
    w.setContentView(new WebContentsView({}));
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
    new WebContentsView({});
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
      const w2 = new BaseWindow();
      w2.setContentView(v);
      // A visibilitychange event is triggered, because the page cycled from
      // visible -> hidden -> visible, but the page's JS can't observe the
      // 'hidden' state.
      expect(await p).to.equal('visible');
    });
  });
});
