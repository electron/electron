import { BrowserWindow, webContents } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import { setTimeout } from 'node:timers/promises';

const windows = new Set<BrowserWindow>();
const contents = new Set<Electron.WebContents>();
async function waitFor(predicate: () => boolean | Promise<boolean>) {
  for (let i = 0; i < 400; ++i) {
    if (await predicate()) return;
    await setTimeout(25);
  }
  throw new Error('webview2 state transition timed out');
}
async function createContents() {
  const page = webContents.create({ type: 'webview2' });
  contents.add(page);
  await page.loadURL('data:text/html,<script>window.persistentMarker=123</script>guest');
  return page;
}
async function createHost(preferences: Electron.WebPreferences = {}) {
  const window = new BrowserWindow({ show: false, webPreferences: { webview2Tag: true, ...preferences } });
  windows.add(window);
  await window.loadURL('data:text/html,<body>host<script>window.transitions=[]</script>');
  await waitFor(() => window.webContents.executeJavaScript("!!customElements.get('webview2')"));
  return window;
}
async function attach(window: BrowserWindow, page: Electron.WebContents) {
  await window.webContents.executeJavaScript(`(() => {
    const element = document.createElement('webview2');
    element.setAttribute('web-contents-id', '${page.id}');
    for (const name of ['attached', 'detached']) element.addEventListener(name,
      event => window.transitions.push({name, ...event.detail}));
    window.view = element;
    document.body.append(element);
  })()`);
  await waitFor(() => page.isAttached());
  await waitFor(() => window.webContents.executeJavaScript("transitions.some(event => event.name === 'attached')"));
}
afterEach(() => {
  for (const window of windows) if (!window.isDestroyed()) window.destroy();
  for (const page of contents) if (!page.isDestroyed()) page.destroy();
  windows.clear();
  contents.clear();
});

describe('webview2 independent lifetime', () => {
  it('loads and runs a page without ever attaching to an iframe', async () => {
    const page = webContents.create({ type: 'webview2', sandbox: true, contextIsolation: true });
    contents.add(page);
    expect(page.getType()).toBe('webview2');
    expect(page.isAttached()).toBe(false);
    expect(page.hostWebContents).toBeNull();
    await page.loadURL(
      'data:text/html,' +
        encodeURIComponent(`
      <title>Unattached page</title><div id="value">pending</div>
      <script>
        window.pageState = { count: 7 };
        document.getElementById('value').textContent = 'page script ran';
      </script>
    `)
    );
    expect(page.isAttached()).toBe(false);
    expect(page.getTitle()).toBe('Unattached page');
    expect(await page.executeJavaScript("document.getElementById('value').textContent")).toBe('page script ran');
    expect(await page.executeJavaScript('Promise.resolve(++pageState.count)')).toBe(8);
    expect(await page.executeJavaScript('pageState.count')).toBe(8);
    expect(page.detach()).toBe(false);
    expect(await page.executeJavaScript('pageState.count')).toBe(8);
    expect(page.isAttached()).toBe(false);
    page.destroy();
    await waitFor(() => page.isDestroyed());
  });

  for (const preferences of [
    { sandbox: true, contextIsolation: true },
    { sandbox: false, contextIsolation: true, nodeIntegration: true },
    { sandbox: false, contextIsolation: false, nodeIntegration: true }
  ]) {
    it(`attaches with ${JSON.stringify(preferences)}`, async () => {
      const page = await createContents();
      const window = await createHost(preferences);
      await attach(window, page);
      expect(await page.executeJavaScript('persistentMarker')).toBe(123);
      expect(page.detach()).toBe(true);
      expect(page.detach()).toBe(false);
      await waitFor(() =>
        window.webContents.executeJavaScript("transitions.filter(event => event.name === 'detached').length === 1")
      );
      await window.webContents.executeJavaScript('view.remove()');
      expect(page.isDestroyed()).toBe(false);
      expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    });
  }

  it('preserves contents when the iframe is removed first', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    await window.webContents.executeJavaScript('view.remove()');
    await waitFor(() => !page.isAttached());
    expect(page.detach()).toBe(false);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    await attach(window, page);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('preserves the element when the contents is destroyed first', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    page.destroy();
    await waitFor(() => page.isDestroyed());
    await waitFor(() => window.webContents.executeJavaScript("transitions.some(event => event.name === 'detached')"));
    expect(await window.webContents.executeJavaScript('view.isConnected')).toBe(true);
    const replacement = await createContents();
    await window.webContents.executeJavaScript(`view.setAttribute('web-contents-id', '${replacement.id}')`);
    await waitFor(() => replacement.isAttached());
  });

  it('transfers to another window and ignores the old element release', async () => {
    const page = await createContents();
    const first = await createHost();
    const second = await createHost();
    await attach(first, page);
    page.detach();
    await attach(second, page);
    await first.webContents.executeJavaScript('view.remove()');
    first.destroy();
    expect(page.isAttached()).toBe(true);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    second.destroy();
    await waitFor(() => !page.isAttached());
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('detaches synchronously when an element is removed immediately', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript(`(() => {
      const element = document.createElement('webview2');
      element.setAttribute('web-contents-id', '${page.id}');
      document.body.append(element);
      element.remove();
    })()`);
    await setTimeout(200);
    expect(page.isAttached()).toBe(false);
    expect(page.isDestroyed()).toBe(false);
  });

  it('automatically detaches when a raw iframe is destroyed', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    await waitFor(() => window.webContents.mainFrame.frames.length === 1);
    const frame = window.webContents.mainFrame.frames[0];
    const attachmentId = page.attach(window.webContents.mainFrame, frame.frameToken);
    expect(attachmentId).toBeTruthy();
    expect(page.isAttached()).toBe(true);
    await window.webContents.executeJavaScript("document.querySelector('iframe').remove()");
    await waitFor(() => !page.isAttached());
    expect(page.isDestroyed()).toBe(false);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('rejects navigated iframe targets without changing either lifetime', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript(
      "const frame = document.createElement('iframe'); frame.srcdoc = 'occupied'; document.body.append(frame)"
    );
    await waitFor(() => window.webContents.mainFrame.frames.some((frame) => frame.url === 'about:srcdoc'));
    const frame = window.webContents.mainFrame.frames[0];
    expect(() => page.attach(window.webContents.mainFrame, frame.frameToken)).toThrow(/iframe/);
    expect(page.isAttached()).toBe(false);
    expect(page.isDestroyed()).toBe(false);
    expect(await window.webContents.executeJavaScript("document.querySelector('iframe').isConnected")).toBe(true);
  });

  it('updates native relationships synchronously between consecutive calls', async () => {
    const oldPage = await createContents();
    const newPage = await createContents();
    const window = await createHost();
    await attach(window, oldPage);
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    await waitFor(() => window.webContents.mainFrame.frames.some((frame) => frame.url === 'about:blank'));
    const frame = window.webContents.mainFrame.frames.find((frame) => frame.url === 'about:blank')!;
    expect(oldPage.detach()).toBe(true);
    const attachmentId = newPage.attach(window.webContents.mainFrame, frame.frameToken);
    expect(oldPage.isAttached()).toBe(false);
    expect(newPage.isAttached()).toBe(true);
    expect(attachmentId).toBeTruthy();
  });

  it('rejects attaching another contents to an occupied iframe', async () => {
    const first = await createContents();
    const second = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    const owner = window.webContents.mainFrame;
    const token = owner.frames[0].frameToken;
    first.attach(owner, token);
    expect(() => second.attach(owner, token)).toThrow(/iframe/);
    expect(first.isAttached()).toBe(true);
    expect(second.isAttached()).toBe(false);
    expect(second.isDestroyed()).toBe(false);
  });

  it('rejects a forged frame token', async () => {
    const page = await createContents();
    const window = await createHost();
    expect(() => page.attach(window.webContents.mainFrame, 'forged')).toThrow(/token/);
    expect(page.isAttached()).toBe(false);
  });
});
