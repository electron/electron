import { BaseWindow, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import { randomUUID } from 'node:crypto';
import { once } from 'node:events';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { setImmediate } from 'node:timers/promises';

import { ciGpuArgs, defer, ifdescribe, spawnAndWait } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

for (const Window of [BaseWindow, BrowserWindow]) {
  ifdescribe(process.platform === 'darwin')(`${Window.name} native tab groups`, { tags: ['serial'] }, () => {
    afterEach(closeAllWindows);

    function createWindow(tabbingIdentifier: string = randomUUID(), disallowAutomaticTabs = false): BaseWindow {
      return new Window({
        show: false,
        tabbingIdentifier,
        tabbingMode: disallowAutomaticTabs ? 'disallowed' : 'automatic'
      });
    }

    function createGroup(count: number) {
      const identifier = randomUUID();
      const tabs = Array.from({ length: count }, () => createWindow(identifier, true));
      tabs[0].showInactive();
      for (let i = 1; i < tabs.length; i++) {
        tabs[i - 1].tabbingMode = 'automatic';
        tabs[i].tabbingMode = 'automatic';
        tabs[i - 1].addTabbedWindow(tabs[i]);
      }
      tabs[0].tabbingMode = 'automatic';
      return tabs;
    }

    it('uses the existing default modes when no tabbingMode is provided', () => {
      const plain = new Window({ show: false });
      const enabled = new Window({ show: false, tabbingIdentifier: randomUUID() });
      expect([plain.tabbingMode, enabled.tabbingMode]).to.deep.equal(['disallowed', 'automatic']);
    });

    it('sets the initial native tabbing mode before showing the window', () => {
      for (const tabbingMode of ['automatic', 'preferred', 'disallowed'] as const) {
        const window = new Window({ show: false, tabbingIdentifier: randomUUID(), tabbingMode });
        expect(window.tabbingMode).to.equal(tabbingMode);
      }
    });

    it('changes native tabbing mode without losing the configured identifier', () => {
      const identifier = randomUUID();
      const window = createWindow(identifier);
      for (const mode of ['preferred', 'disallowed', 'automatic'] as const) {
        window.tabbingMode = mode;
        expect(window.tabbingMode).to.equal(mode);
      }
      expect(window.tabbingIdentifier).to.equal(identifier);
    });

    it('rejects invalid constructor modes without creating a native window', () => {
      const before = BaseWindow.getAllWindows();
      for (const tabbingMode of ['invalid', '', true, 42, null]) {
        expect(() => Reflect.construct(Window, [{ show: false, tabbingMode }])).to.throw('Invalid tabbing mode');
      }
      expect(BaseWindow.getAllWindows()).to.deep.equal(before);
    });

    it('rejects invalid property modes without changing the native mode', () => {
      const window = createWindow(randomUUID(), true);
      for (const mode of ['invalid', '', true, 42, null]) {
        expect(() => Reflect.set(window, 'tabbingMode', mode)).to.throw();
        expect(window.tabbingMode).to.equal('disallowed');
      }
    });

    it('returns no group for a window without native tabs', () => {
      const window = new Window({ show: false });
      expect(window.getTabbedWindows()).to.deep.equal([]);
      expect(window.getSelectedTab()).to.equal(null);
      expect(() => window.selectTab()).not.to.throw();
    });

    it('returns the same ordered group from every member', () => {
      const tabs = createGroup(3);
      for (const tab of tabs) {
        expect(tab.getTabbedWindows()).to.deep.equal(tabs);
      }
    });

    it('returns a fresh snapshot and ignores modifications to returned arrays', () => {
      const tabs = createGroup(2);
      const snapshot = tabs[0].getTabbedWindows();
      const third = createWindow();
      tabs[1].addTabbedWindow(third);

      expect(snapshot).to.deep.equal(tabs);
      snapshot.length = 0;
      expect(tabs[0].getTabbedWindows()).to.deep.equal([...tabs, third]);
    });

    it('reports the selected tab from every member', () => {
      const tabs = createGroup(3);
      for (const selected of tabs) {
        selected.selectTab();
        expect(tabs.map((tab) => tab.getSelectedTab())).to.deep.equal(tabs.map(() => selected));
      }
    });

    it('selects a tab without focusing an inactive group', async () => {
      const tabs = createGroup(2);
      const other = createWindow();
      const focused = once(other, 'focus');
      other.show();
      await focused;
      expect(BaseWindow.getFocusedWindow()).to.equal(other);

      tabs[0].selectTab();
      expect(tabs[1].getSelectedTab()).to.equal(tabs[0]);
      expect(BaseWindow.getFocusedWindow()).to.equal(other);
    });

    it('reflects a changed tab order', () => {
      const [first, second, third] = createGroup(3);
      first.addTabbedWindow(third);

      expect(second.getTabbedWindows()).to.deep.equal([first, third, second]);
    });

    it('keeps independent groups separate even with the same tabbing identifier', () => {
      const [first, second] = createGroup(2);
      const third = createWindow(first.tabbingIdentifier);
      const fourth = createWindow(first.tabbingIdentifier);
      third.addTabbedWindow(fourth);

      expect(first.getTabbedWindows()).to.deep.equal([first, second]);
      expect(fourth.getTabbedWindows()).to.deep.equal([third, fourth]);
    });

    it('reflects detaching a tab and joining a different group', async () => {
      const [first, second, third] = createGroup(3);
      const [fourth, fifth] = createGroup(2);
      second.moveTabToNewWindow();
      await setImmediate();
      fourth.addTabbedWindow(second);

      expect(first.getTabbedWindows()).to.deep.equal([first, third]);
      expect(fifth.getTabbedWindows()).to.deep.equal([fourth, second, fifth]);
    });

    it('reflects merging independent groups', async () => {
      const identifier = randomUUID();
      const tabs = Array.from({ length: 4 }, () => createWindow(identifier, true));
      tabs[0].showInactive();
      tabs[0].tabbingMode = 'automatic';
      tabs[1].tabbingMode = 'automatic';
      tabs[0].addTabbedWindow(tabs[1]);
      tabs[2].showInactive();
      tabs[2].tabbingMode = 'automatic';
      tabs[3].tabbingMode = 'automatic';
      tabs[2].addTabbedWindow(tabs[3]);
      await setImmediate();
      expect(tabs[0].getTabbedWindows()).to.deep.equal(tabs.slice(0, 2));
      expect(tabs[2].getTabbedWindows()).to.deep.equal(tabs.slice(2));
      tabs[0].mergeAllWindows();
      await setImmediate();

      const merged = tabs[0].getTabbedWindows();
      expect(merged).to.have.members(tabs);
      for (const tab of tabs) expect(tab.getTabbedWindows()).to.deep.equal(merged);
    });

    it('reads a minimized group and restores it when selecting a different tab', async () => {
      const tabs = createGroup(3);
      tabs[1].selectTab();
      const minimized = once(tabs[1], 'minimize');
      tabs[1].minimize();
      await minimized;

      expect(tabs[0].getTabbedWindows()).to.deep.equal(tabs);
      expect(tabs[2].getSelectedTab()).to.equal(tabs[1]);
      const restored = new Promise<void>((resolve) => {
        const onRestore = () => {
          for (const tab of tabs) tab.removeListener('restore', onRestore);
          resolve();
        };
        for (const tab of tabs) tab.once('restore', onRestore);
      });
      tabs[0].selectTab();
      expect(tabs[2].getSelectedTab()).to.equal(tabs[0]);
      await restored;
      await setImmediate();
      expect(tabs.every((tab) => !tab.isMinimized())).to.equal(true);
      // Native restoration may reselect the formerly active tab. Apply the
      // requested selection once restoration has completed.
      tabs[0].selectTab();
      await setImmediate();
      expect(tabs[2].getSelectedTab()).to.equal(tabs[0]);
    });

    it('reads native tabs in fullscreen', async function () {
      this.timeout(30000);
      const directory = await mkdtemp(join(tmpdir(), 'electron-native-tab-fullscreen-'));
      defer(() => rm(directory, { recursive: true, force: true }));
      // A fresh app isolates native Spaces from previous window animations.
      const fixture = join(import.meta.dirname, 'fixtures', 'apps', 'native-tab-session');
      const child = await spawnAndWait(
        process.execPath,
        [
          fixture,
          ...ciGpuArgs,
          '--session-action=fullscreen',
          `--session-window-type=${Window.name}`,
          `--user-data-dir=${join(directory, 'profile')}`
        ],
        { timeout: 25000 }
      );
      expect(child.code, child.stderr).to.equal(0);
      expect(child.stdout).to.include('FULLSCREEN_TABS_OK');
    });

    it('omits a closed tab and updates the selected tab', async () => {
      const [first, second, third] = createGroup(3);
      second.selectTab();
      const closed = once(second, 'closed');
      second.close();
      await closed;

      expect(first.getTabbedWindows()).to.deep.equal([first, third]);
      expect([first, third]).to.include(first.getSelectedTab());
      expect(third.getSelectedTab()).to.equal(first.getSelectedTab());
    });

    it('keeps a group intact when closing is cancelled', () => {
      const tabs = createGroup(3);
      tabs[1].selectTab();
      tabs[1].on('close', (event) => event.preventDefault());
      tabs[1].close();

      expect(tabs[0].getTabbedWindows()).to.deep.equal(tabs);
      expect(tabs[0].getSelectedTab()).to.equal(tabs[1]);
    });

    it('omits closed wrappers when queried from the closed event', async () => {
      const [first, second, third] = createGroup(3);
      second.selectTab();
      let snapshot: BaseWindow[] | undefined;
      let selected: BaseWindow | null | undefined;
      second.on('closed', () => {
        snapshot = first.getTabbedWindows();
        selected = first.getSelectedTab();
      });
      const closed = once(second, 'closed');
      second.close();
      await closed;

      expect(snapshot).to.deep.equal([first, third]);
      expect(selected).not.to.equal(second);
    });

    it('captures the complete group in the close event before native removal', async () => {
      const tabs = createGroup(3);
      tabs[1].selectTab();
      const expected = { windows: tabs.map((tab) => tab.id), selected: tabs[1].id };
      let captured: { windows: number[]; selected: number | undefined } | undefined;
      tabs[1].on('close', () => {
        captured = {
          windows: tabs[1].getTabbedWindows().map((tab) => tab.id),
          selected: tabs[1].getSelectedTab()?.id
        };
      });
      const closed = once(tabs[1], 'closed');
      tabs[1].close();
      await closed;

      expect(captured).to.deep.equal(expected);
    });

    it('restores separate groups, tab order and selection after closing all windows', async () => {
      const groups = [createGroup(3), createGroup(2)];
      groups[0][1].selectTab();
      groups[1][0].selectTab();
      let label = 0;
      for (const tabs of groups) {
        for (const tab of tabs) tab.setTitle(`workspace-${label++}`);
      }
      const saved = JSON.parse(
        JSON.stringify(
          groups.map((tabs) => ({
            titles: tabs[0].getTabbedWindows().map((tab) => tab.getTitle()),
            selected: tabs[0].getSelectedTab()?.getTitle()
          }))
        )
      ) as { titles: string[]; selected: string }[];

      for (const tab of groups.flat()) {
        const closed = once(tab, 'closed');
        tab.close();
        await closed;
      }

      const restored = saved.map((group) => {
        const tabs = createGroup(group.titles.length);
        tabs.forEach((tab, index) => tab.setTitle(group.titles[index]));
        tabs.find((tab) => tab.getTitle() === group.selected)!.selectTab();
        return {
          titles: tabs[0].getTabbedWindows().map((tab) => tab.getTitle()),
          selected: tabs[0].getSelectedTab()?.getTitle()
        };
      });
      expect(restored).to.deep.equal(saved);
    });

    it('restores a captured group after hiding every member', async () => {
      const tabs = createGroup(3);
      tabs[1].selectTab();
      const saved = tabs[0].getTabbedWindows();
      const selected = tabs[0].getSelectedTab()!;
      for (const tab of saved) {
        tab.hide();
        await setImmediate();
      }

      saved[0].tabbingMode = 'disallowed';
      saved[0].showInactive();
      await setImmediate();
      for (let i = 1; i < saved.length; i++) {
        saved[i].tabbingMode = 'disallowed';
        saved[i - 1].tabbingMode = 'automatic';
        saved[i].tabbingMode = 'automatic';
        saved[i - 1].addTabbedWindow(saved[i]);
        await setImmediate();
      }
      selected.selectTab();

      expect(tabs[0].getTabbedWindows()).to.deep.equal(saved);
      expect(tabs[2].getSelectedTab()).to.equal(selected);
    });

    it('rejects calls on destroyed windows', () => {
      const window = createWindow();
      window.destroy();

      expect(() => window.getTabbedWindows()).to.throw();
      expect(() => window.getSelectedTab()).to.throw();
      expect(() => window.selectTab()).to.throw();
      expect(() => window.tabbingMode).to.throw();
      expect(() => {
        window.tabbingMode = 'automatic';
      }).to.throw();
    });

    for (const action of ['quit', 'close', 'native-close', 'hide-quit']) {
      it(`restores tab groups in a new process after ${action}`, async function () {
        this.timeout(60000);
        const directory = await mkdtemp(join(tmpdir(), 'electron-native-tab-session-'));
        defer(() => rm(directory, { recursive: true, force: true }));
        const statePath = join(directory, 'tabs.json');
        const fixture = join(import.meta.dirname, 'fixtures', 'apps', 'native-tab-session');
        const args = [
          fixture,
          ...ciGpuArgs,
          `--session-state=${statePath}`,
          `--session-window-type=${Window.name}`,
          `--user-data-dir=${join(directory, 'profile')}`
        ];
        const savedProcess = await spawnAndWait(process.execPath, [...args, `--session-action=${action}`], {
          timeout: 25000
        });
        expect(savedProcess.code, savedProcess.stderr).to.equal(0);
        const saved = JSON.parse(await readFile(statePath, 'utf8'));

        const restoredProcess = await spawnAndWait(process.execPath, [...args, '--session-action=restore'], {
          timeout: 25000
        });
        expect(restoredProcess.code, restoredProcess.stderr).to.equal(0);
        const output = restoredProcess.stdout.split('\n').find((line) => line.startsWith('NATIVE_TABS_STATE='));
        expect(output, restoredProcess.stdout).to.be.a('string');
        expect(JSON.parse(output!.slice('NATIVE_TABS_STATE='.length))).to.deep.equal(saved);
      });
    }
  });
}

ifdescribe(process.platform === 'darwin')('mixed native tab groups', { tags: ['serial'] }, () => {
  afterEach(closeAllWindows);

  it('returns the original BaseWindow and BrowserWindow wrappers', () => {
    const identifier = randomUUID();
    const base = new BaseWindow({ show: false, tabbingIdentifier: identifier });
    const browser = new BrowserWindow({ show: false, tabbingIdentifier: identifier });
    base.showInactive();
    base.addTabbedWindow(browser);
    browser.selectTab();

    expect(base.getTabbedWindows()).to.deep.equal([base, browser]);
    expect(browser.getTabbedWindows()).to.deep.equal([base, browser]);
    expect(base.getSelectedTab()).to.equal(browser);
    expect(browser.getSelectedTab()).to.equal(browser);
  });
});
