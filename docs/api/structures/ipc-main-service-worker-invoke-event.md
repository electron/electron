# IpcMainServiceWorkerInvokeEvent Object extends `Event`

* `type` String - Possible values include `service-worker`.
* `serviceWorker` [ServiceWorkerMain](../service-worker-main.md) _Readonly_ - The service worker that sent this message
* `versionId` Number - The service worker version ID.
* `session` Session - The [`Session`](../session.md) instance with which the event is associated.
