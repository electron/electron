import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { webContents } from 'electron/main';

interface Slot {
  key: string;
  owner: Electron.WebFrameMain;
  embedder: Electron.WebContents;
  contents: Electron.WebContents;
  attachmentId?: string;
  terminal?: string;
  onDestroyed: () => void;
  expiry?: ReturnType<typeof setTimeout>;
  onDetached: (event: Electron.Event, id: string) => void;
}
const slots = new Map<string, Slot>();
const contentsSlots = new Map<number, Slot>();
const watched = new WeakSet<Electron.WebContents>();

function key(event: Electron.IpcMainEvent | Electron.IpcMainInvokeEvent, slot: string) {
  const owner = event.senderFrame;
  if (!owner || owner !== event.sender.mainFrame || owner.detached || !/^[0-9]+:[0-9]+$/.test(slot)) {
    throw new Error('Invalid webview2 slot');
  }
  return `${event.sender.id}:${owner.processId}:${owner.routingId}:${slot}`;
}
function remove(record: Slot, detach: boolean) {
  slots.delete(record.key);
  if (contentsSlots.get(record.contents.id) === record) {
    contentsSlots.delete(record.contents.id);
    if (detach && !record.contents.isDestroyed()) record.contents.detach();
  }
  clearTimeout(record.expiry);
  record.contents.removeListener('destroyed', record.onDestroyed);
  record.contents.removeListener('-webview2-detached' as any, record.onDetached);
}
function watch(embedder: Electron.WebContents) {
  if (watched.has(embedder)) return;
  watched.add(embedder);
  const clear = () => {
    for (const record of [...slots.values()]) {
      if (record.embedder === embedder) remove(record, true);
    }
  };
  embedder.once('will-destroy' as any, clear);
  embedder.on('render-process-gone', clear);
  embedder.on('did-start-navigation', (_event, _url, _inPlace, isMainFrame) => {
    if (isMainFrame && !_inPlace) clear();
  });
}

ipcMainUtils.handleSync(IPC_MESSAGES.WEBVIEW2_ATTACH, (event, slot: string, id: number, frame: string) => {
  if (!('sender' in event)) throw new Error('webview2 requires a frame');
  const slotKey = key(event, slot);
  if (!event.sender.getLastWebPreferences()?.webview2Tag) throw new Error('webview2Tag is disabled');
  const contents = webContents.fromId(id);
  if (!contents || contents.isDestroyed() || String(contents.getType()) !== 'webview2') {
    throw new Error('Invalid webview2 WebContents');
  }
  const oldSlot = contentsSlots.get(id);
  if (oldSlot?.attachmentId && !contents.isAttached()) contentsSlots.delete(id);
  if (slots.has(slotKey) || contentsSlots.has(id) || contents.isAttached()) {
    throw new Error('WebContents or slot is already reserved');
  }
  const record: Slot = {
    key: slotKey,
    owner: event.senderFrame!,
    embedder: event.sender,
    contents,
    onDestroyed: () => {
      clearTimeout(record.expiry);
      record.expiry = setTimeout(() => {
        if (slots.get(slotKey) === record) remove(record, false);
      }, 30000);
      if (record.attachmentId && !record.terminal && !record.owner.detached) {
        record.terminal = record.attachmentId;
        record.embedder._sendInternal(IPC_MESSAGES.WEBVIEW2_DETACHED, slot, id, record.attachmentId);
      }
      if (contentsSlots.get(id) === record) contentsSlots.delete(id);
    },
    onDetached: (_event, attachmentId) => {
      if (record.attachmentId !== attachmentId) return;
      record.terminal = attachmentId;
      clearTimeout(record.expiry);
      record.expiry = setTimeout(() => {
        if (slots.get(slotKey) === record) remove(record, false);
      }, 30000);
      if (contentsSlots.get(id) === record) contentsSlots.delete(id);
      if (!record.owner.detached) {
        record.embedder._sendInternal(IPC_MESSAGES.WEBVIEW2_DETACHED, slot, id, attachmentId);
      }
    }
  };
  slots.set(slotKey, record);
  contentsSlots.set(id, record);
  watch(event.sender);
  contents.on('-webview2-detached' as any, record.onDetached);
  contents.once('destroyed', record.onDestroyed);
  try {
    record.attachmentId = contents.attach(record.owner, frame);
    return { webContentsId: id, attachmentId: record.attachmentId };
  } catch (error) {
    remove(record, false);
    throw error;
  }
});

ipcMainUtils.handleSync(IPC_MESSAGES.WEBVIEW2_RELEASE, (event, slot: string) => {
  if (!('sender' in event)) throw new Error('webview2 requires a frame');
  const record = slots.get(key(event, slot));
  if (!record) return null;
  const receipt = record.attachmentId ? { webContentsId: record.contents.id, attachmentId: record.attachmentId } : null;
  remove(record, true);
  return receipt;
});
