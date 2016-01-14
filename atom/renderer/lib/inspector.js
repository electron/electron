var convertToMenuTemplate, createFileSelectorElement, createMenu, pathToHtml5FileObject, showFileChooserDialog;

window.onload = function() {
  // Use menu API to show context menu.
  InspectorFrontendHost.showContextMenuAtPoint = createMenu;

  // Use dialog API to override file chooser dialog.
  return WebInspector.createFileSelectorElement = createFileSelectorElement;
};

convertToMenuTemplate = function(items) {
  var fn, i, item, len, template;
  template = [];
  fn = function(item) {
    var transformed;
    transformed = item.type === 'subMenu' ? {
      type: 'submenu',
      label: item.label,
      enabled: item.enabled,
      submenu: convertToMenuTemplate(item.subItems)
    } : item.type === 'separator' ? {
      type: 'separator'
    } : item.type === 'checkbox' ? {
      type: 'checkbox',
      label: item.label,
      enabled: item.enabled,
      checked: item.checked
    } : {
      type: 'normal',
      label: item.label,
      enabled: item.enabled
    };
    if (item.id != null) {
      transformed.click = function() {
        DevToolsAPI.contextMenuItemSelected(item.id);
        return DevToolsAPI.contextMenuCleared();
      };
    }
    return template.push(transformed);
  };
  for (i = 0, len = items.length; i < len; i++) {
    item = items[i];
    fn(item);
  }
  return template;
};

createMenu = function(x, y, items, document) {
  var Menu, menu, remote;
  remote = require('electron').remote;
  Menu = remote.Menu;
  menu = Menu.buildFromTemplate(convertToMenuTemplate(items));

  // The menu is expected to show asynchronously.
  return setTimeout(function() {
    return menu.popup(remote.getCurrentWindow());
  });
};

showFileChooserDialog = function(callback) {
  var dialog, files, remote;
  remote = require('electron').remote;
  dialog = remote.dialog;
  files = dialog.showOpenDialog({});
  if (files != null) {
    return callback(pathToHtml5FileObject(files[0]));
  }
};

pathToHtml5FileObject = function(path) {
  var blob, fs;
  fs = require('fs');
  blob = new Blob([fs.readFileSync(path)]);
  blob.name = path;
  return blob;
};

createFileSelectorElement = function(callback) {
  var fileSelectorElement;
  fileSelectorElement = document.createElement('span');
  fileSelectorElement.style.display = 'none';
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback);
  return fileSelectorElement;
};
