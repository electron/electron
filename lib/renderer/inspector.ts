import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { IPC_MESSAGES } from '../common/ipc-messages';

window.onload = function () {
  // Use menu API to show context menu.
  window.InspectorFrontendHost!.showContextMenuAtPoint = createMenu;

  // correct for Chromium returning undefined for filesystem
  window.Persistence!.FileSystemWorkspaceBinding.completeURL = completeURL;

  // Use dialog API to override file chooser dialog.
  window.UI!.createFileSelectorElement = createFileSelectorElement;
};

// Extra / is needed as a result of MacOS requiring absolute paths
function completeURL (project: string, path: string) {
  project = 'file:///';
  return `${project}${path}`;
}

// The DOM implementation expects (message?: string) => boolean
window.confirm = function (message?: string, title?: string) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.INSPECTOR_CONFIRM, message, title) as boolean;
};

const useEditMenuItems = function (x: number, y: number, items: ContextMenuItem[]) {
  return items.length === 0 && document.elementsFromPoint(x, y).some(function (element) {
    return element.nodeName === 'INPUT' ||
      element.nodeName === 'TEXTAREA' ||
      (element as HTMLElement).isContentEditable;
  });
};

const createMenu = function (x: number, y: number, items: ContextMenuItem[]) {
  const isEditMenu = useEditMenuItems(x, y, items);
  ipcRendererInternal.invoke<number>(IPC_MESSAGES.INSPECTOR_CONTEXT_MENU, items, isEditMenu).then(id => {
    if (typeof id === 'number') {
      window.DevToolsAPI!.contextMenuItemSelected(id);
    }
    window.DevToolsAPI!.contextMenuCleared();
  });
};

const showFileChooserDialog = function (callback: (blob: File) => void) {
  ipcRendererInternal.invoke<[ string, any ]>(IPC_MESSAGES.INSPECTOR_SELECT_FILE).then(([path, data]) => {
    if (path && data) {
      callback(dataToHtml5FileObject(path, data));
    }
  });
};

const dataToHtml5FileObject = function (path: string, data: any) {
  return new File([data], path);
};

const createFileSelectorElement = function (this: any, callback: () => void) {
  const fileSelectorElement = document.createElement('span');
  fileSelectorElement.style.display = 'none';
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback);
  return fileSelectorElement;
};
