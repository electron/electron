'use strict';

import { WebContents } from 'electron/main';

const v8Util = process._linkedBinding('electron_common_v8_util');

const getOwnerKey = (webContents: WebContents, contextId: string) => {
  return `${webContents.id}-${contextId}`;
};

class ObjectsRegistry {
  private nextId: number = 0

  // Stores all objects by ref-counting.
  // (id) => {object, count}
  private storage: Record<number, { count: number, object: any }> = {}

  // Stores the IDs + refCounts of objects referenced by WebContents.
  // (ownerKey) => { id: refCount }
  private owners: Record<string, Map<number, number>> = {}

  // Register a new object and return its assigned ID. If the object is already
  // registered then the already assigned ID would be returned.
  add (webContents: WebContents, contextId: string, obj: any) {
    // Get or assign an ID to the object.
    const id = this.saveToStorage(obj);

    // Add object to the set of referenced objects.
    const ownerKey = getOwnerKey(webContents, contextId);
    let owner = this.owners[ownerKey];
    if (!owner) {
      owner = this.owners[ownerKey] = new Map();
      this.registerDeleteListener(webContents, contextId);
    }
    if (!owner.has(id)) {
      owner.set(id, 0);
      // Increase reference count if not referenced before.
      this.storage[id].count++;
    }

    owner.set(id, owner.get(id)! + 1);
    return id;
  }

  // Get an object according to its ID.
  get (id: number) {
    const pointer = this.storage[id];
    if (pointer != null) return pointer.object;
  }

  // Dereference an object according to its ID.
  // Note that an object may be double-freed (cleared when page is reloaded, and
  // then garbage collected in old page).
  remove (webContents: WebContents, contextId: string, id: number) {
    const ownerKey = getOwnerKey(webContents, contextId);
    const owner = this.owners[ownerKey];
    if (owner && owner.has(id)) {
      const newRefCount = owner.get(id)! - 1;

      // Only completely remove if the number of references GCed in the
      // renderer is the same as the number of references we sent them
      if (newRefCount <= 0) {
        // Remove the reference in owner.
        owner.delete(id);
        // Dereference from the storage.
        this.dereference(id);
      } else {
        owner.set(id, newRefCount);
      }
    }
  }

  // Clear all references to objects refrenced by the WebContents.
  clear (webContents: WebContents, contextId: string) {
    const ownerKey = getOwnerKey(webContents, contextId);
    const owner = this.owners[ownerKey];
    if (!owner) return;

    for (const id of owner.keys()) this.dereference(id);

    delete this.owners[ownerKey];
  }

  // Private: Saves the object into storage and assigns an ID for it.
  saveToStorage (object: any) {
    let id: number = v8Util.getHiddenValue(object, 'electronId');
    if (!id) {
      id = ++this.nextId;
      this.storage[id] = {
        count: 0,
        object: object
      };
      v8Util.setHiddenValue(object, 'electronId', id);
    }
    return id;
  }

  // Private: Dereference the object from store.
  dereference (id: number) {
    const pointer = this.storage[id];
    if (pointer == null) {
      return;
    }
    pointer.count -= 1;
    if (pointer.count === 0) {
      v8Util.deleteHiddenValue(pointer.object, 'electronId');
      delete this.storage[id];
    }
  }

  // Private: Clear the storage when renderer process is destroyed.
  registerDeleteListener (webContents: WebContents, contextId: string) {
    // contextId => ${processHostId}-${contextCount}
    const processHostId = contextId.split('-')[0];
    const listener = (_: any, deletedProcessHostId: string) => {
      if (deletedProcessHostId &&
          deletedProcessHostId.toString() === processHostId) {
        webContents.removeListener('render-view-deleted' as any, listener);
        this.clear(webContents, contextId);
      }
    };
    // Note that the "render-view-deleted" event may not be emitted on time when
    // the renderer process get destroyed because of navigation, we rely on the
    // renderer process to send "ELECTRON_BROWSER_CONTEXT_RELEASE" message to
    // guard this situation.
    webContents.on('render-view-deleted' as any, listener);
  }
}

export default new ObjectsRegistry();
