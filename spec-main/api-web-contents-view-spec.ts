import { expect } from 'chai';
import * as ChildProcess from 'child_process';
import * as path from 'path';
import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';

import { BaseWindow, WebContentsView } from 'electron/main';

describe('WebContentsView', () => {
  let w: BaseWindow;
  afterEach(() => closeWindow(w as any).then(() => { w = null as unknown as BaseWindow; }));

  it('can be used as content view', () => {
    w = new BaseWindow({ show: false });
    w.setContentView(new WebContentsView({}));
  });

  describe('new WebContentsView()', () => {
    it('does not crash on exit', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'leak-exit-webcontentsview.js');
      const electronPath = process.execPath;
      const appProcess = ChildProcess.spawn(electronPath, ['--enable-logging', appPath]);
      let output = '';
      appProcess.stdout.on('data', data => { output += data; });
      appProcess.stderr.on('data', data => { output += data; });
      const [code] = await emittedOnce(appProcess, 'exit');
      if (code !== 0) {
        console.log(code, output);
      }
      expect(code).to.equal(0);
    });
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
});
