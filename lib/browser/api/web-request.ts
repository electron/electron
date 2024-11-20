import * as deprecate from '@electron/internal/common/deprecate';

const { WebRequest } = process._linkedBinding('electron_browser_web_request');

const urlsDeprecated = deprecate.warnOnce('WebRequestFilter.urls', 'WebRequestFilter.includeUrls');

function modifyArgs (args: any[]) {
  if (args.length < 2) return;
  const filter = args[0];

  if (!filter?.urls) {
    return;
  }

  urlsDeprecated();
  if (filter.urls.length === 0) {
    filter.includeUrls = ['<all_urls>'];
  } else {
    filter.includeUrls = filter.urls;
  }

  delete filter.urls;
}

const onBeforeRequest = WebRequest.prototype.onBeforeRequest;
WebRequest.prototype.onBeforeRequest = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onBeforeRequest.apply(this, args);
};

const onBeforeSendHeaders = WebRequest.prototype.onBeforeSendHeaders;
WebRequest.prototype.onBeforeSendHeaders = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onBeforeSendHeaders.apply(this, args);
};

const onSendHeaders = WebRequest.prototype.onSendHeaders;
WebRequest.prototype.onSendHeaders = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onSendHeaders.apply(this, args);
};

const onHeadersReceived = WebRequest.prototype.onHeadersReceived;
WebRequest.prototype.onHeadersReceived = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onHeadersReceived.apply(this, args);
};

const onResponseStarted = WebRequest.prototype.onResponseStarted;
WebRequest.prototype.onResponseStarted = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onResponseStarted.apply(this, args);
};

const onBeforeRedirect = WebRequest.prototype.onBeforeRedirect;
WebRequest.prototype.onBeforeRedirect = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onBeforeRedirect.apply(this, args);
};

const onCompleted = WebRequest.prototype.onCompleted;
WebRequest.prototype.onCompleted = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onCompleted.apply(this, args);
};

const onErrorOccurred = WebRequest.prototype.onErrorOccurred;
WebRequest.prototype.onErrorOccurred = function (this: Electron.WebRequest, ...args: any[]) {
  modifyArgs(args);
  return onErrorOccurred.apply(this, args);
};
