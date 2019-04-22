## Modernization

The Electron team is currently undergoing an initiative to modernize our API in a few concrete ways. These include: updating our modules to use idiomatic JS properties instead of separate `getPropertyX` and `setpropertyX`, converting callbacks to promises, and removing some other anti-patterns present in our APIs. The current status of the Promise intiative can be tracked in the [promisification](promisification.md) tracking file.

As we work to perform these updates, we seek to create the least disruptive amount of change at any given time, so as many changes as possible will be introduced in a backward compatible manner and deprecated after enough time has passed to give users a chance to upgrade their API calls.

This document and its child documents will be updated to reflect the latest status of our API changes.

* [Promisification](promisification.md)
* [Property Updates](property-updates.md)
