const { ServiceWorkerMain } = process._linkedBinding('electron_browser_service_worker_main');

ServiceWorkerMain.prototype.startTask = function () {
  // TODO(samuelmaddock): maybe make timeout configurable in the future
  const hasTimeout = false;
  const { id, ok } = this._startExternalRequest(hasTimeout);

  if (!ok) {
    throw new Error('Unable to start service worker task.');
  }

  return {
    end: () => this._finishExternalRequest(id)
  };
};

module.exports = ServiceWorkerMain;
