const { app, BaseWindow, BrowserWindow, Menu } = require('electron');

const assert = require('node:assert/strict');
const { randomUUID } = require('node:crypto');
const { once } = require('node:events');
const { readFileSync, writeFileSync } = require('node:fs');
const { setImmediate, setTimeout } = require('node:timers/promises');

const statePath = app.commandLine.getSwitchValue('session-state');
const action = app.commandLine.getSwitchValue('session-action');
const Window = app.commandLine.getSwitchValue('session-window-type') === 'BaseWindow' ? BaseWindow : BrowserWindow;
// A single application identifier must still allow distinct saved tab groups.
const tabbingIdentifier = randomUUID();

async function createGroup(titles, selected) {
  const tabs = titles.map((title) => {
    const tab = new Window({ show: false, tabbingIdentifier, tabbingMode: 'disallowed' });
    tab.setTitle(title);
    return tab;
  });
  tabs[0].showInactive();
  await setImmediate();
  for (let i = 1; i < tabs.length; i++) {
    tabs[i - 1].tabbingMode = 'automatic';
    tabs[i].tabbingMode = 'automatic';
    tabs[i - 1].addTabbedWindow(tabs[i]);
    await setImmediate();
  }
  tabs.find((tab) => tab.getTitle() === selected).selectTab();
  return tabs;
}

function capture(groups) {
  return groups.map((tabs) => ({
    titles: tabs[0].getTabbedWindows().map((tab) => tab.getTitle()),
    selected: tabs[0].getSelectedTab()?.getTitle()
  }));
}

app
  .whenReady()
  .then(async () => {
    const session =
      action === 'restore'
        ? JSON.parse(readFileSync(statePath, 'utf8'))
        : [
            { titles: ['workspace-c', 'workspace-a', 'workspace-b'], selected: 'workspace-a' },
            { titles: ['workspace-e', 'workspace-d'], selected: 'workspace-e' }
          ];
    const groups = [];
    for (const group of session) {
      groups.push(await createGroup(group.titles, group.selected));
    }
    assert.deepStrictEqual(capture(groups), session);

    if (action === 'fullscreen') {
      const selected = groups[0][0].getSelectedTab();
      if (!selected.isFocused()) {
        const focused = once(selected, 'focus');
        selected.show();
        app.focus({ steal: true });
        await focused;
      }
      await setTimeout();
      const entered = once(selected, 'enter-full-screen');
      selected.setFullScreen(true);
      await entered;
      assert.equal(selected.isFullScreen(), true);
      assert.deepStrictEqual(capture(groups), session);
      process.stdout.write('FULLSCREEN_TABS_OK\n');
      const left = once(selected, 'leave-full-screen');
      selected.setFullScreen(false);
      await left;
      await setTimeout();
      app.quit();
      return;
    }

    if (action === 'restore') {
      process.stdout.write(`NATIVE_TABS_STATE=${JSON.stringify(capture(groups))}\n`);
      app.quit();
      return;
    }

    if (action === 'hide-quit') {
      // AppKit can detach hidden tabs. Retain the snapshot captured before hiding.
      const retained = capture(groups);
      for (const tab of groups.flat()) {
        tab.hide();
        await setImmediate();
      }
      app.once('before-quit', () => writeFileSync(statePath, JSON.stringify(retained)));
      app.quit();
      return;
    }

    let saved = false;
    const save = () => {
      if (saved) return;
      writeFileSync(statePath, JSON.stringify(capture(groups)));
      saved = true;
    };
    app.once('before-quit', save);
    const tabs = groups.flat();
    for (const tab of tabs) tab.once('close', save);

    if (action === 'quit') {
      app.quit();
      return;
    }

    app.on('window-all-closed', () => app.quit());
    if (action === 'native-close') {
      const selected = groups[0][0].getSelectedTab();
      if (!selected.isFocused()) {
        const focused = once(selected, 'focus');
        selected.focus();
        app.focus({ steal: true });
        await focused;
      }
      const closed = once(selected, 'closed');
      Menu.sendActionToFirstResponder('performClose:');
      await closed;
    }
    for (const tab of tabs) {
      if (tab.isDestroyed()) continue;
      const closed = once(tab, 'closed');
      tab.close();
      await closed;
    }
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
