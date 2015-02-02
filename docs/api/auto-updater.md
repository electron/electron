# auto-updater

**This module has only been implemented for OS X.**

The `auto-updater` module is a simple wrap around the
[Squirrel.Mac](https://github.com/Squirrel/Squirrel.Mac) framework.

Squirrel.Mac requires that your `.app` folder is signed using the
[codesign](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/codesign.1.html)
utility for updates to be installed.

## Squirrel

Squirrel is an OS X framework focused on making application updates **as safe
and transparent as updates to a website**.

Instead of publishing a feed of versions from which your app must select,
Squirrel updates to the version your server tells it to. This allows you to
intelligently update your clients based on the request you give to Squirrel.

Your request can include authentication details, custom headers or a request
body so that your server has the context it needs in order to supply the most
suitable update.

The update JSON Squirrel requests should be dynamically generated based on
criteria in the request, and whether an update is required. Squirrel relies
on server side support for determining whether an update is required, see
[Server Support](#server-support).

Squirrel's installer is also designed to be fault tolerant, and ensure that any
updates installed are valid.

## Update Requests

Squirrel is indifferent to the request the client application provides for
update checking. `Accept: application/json` is added to the request headers
because Squirrel is responsible for parsing the response.

For the requirements imposed on the responses and the body format of an update
response see [Server Support](#server-support).

Your update request must *at least* include a version identifier so that the
server can determine whether an update for this specific version is required. It
may also include other identifying criteria such as operating system version or
username, to allow the server to deliver as fine grained an update as you
would like.

How you include the version identifier or other criteria is specific to the
server that you are requesting updates from. A common approach is to use query
parameters, like this:

```javascript
// On browser side
var app = require('app');
var autoUpdater = require('auto-updater');
autoUpdater.setFeedUrl('http://mycompany.com/myapp/latest?version=' + app.getVersion());
```

## Server Support

Your server should determine whether an update is required based on the
[Update Request](#update-requests) your client issues.

If an update is required your server should respond with a status code of
[200 OK](http://tools.ietf.org/html/rfc2616#section-10.2.1) and include the
[update JSON](#update-json-format) in the body. Squirrel **will** download and
install this update, even if the version of the update is the same as the
currently running version. To save redundantly downloading the same version
multiple times your server must not inform the client to update.

If no update is required your server must respond with a status code of
[204 No Content](http://tools.ietf.org/html/rfc2616#section-10.2.5). Squirrel
will check for an update again at the interval you specify.

## Update JSON Format

When an update is available, Squirrel expects the following schema in response
to the update request provided:

```json
{
  "url": "http://mycompany.com/myapp/releases/myrelease",
  "name": "My Release Name",
  "notes": "Theses are some release notes innit",
  "pub_date": "2013-09-18T12:29:53+01:00",
}
```

The only required key is "url", the others are optional.

Squirrel will request "url" with `Accept: application/zip` and only supports
installing ZIP updates. If future update formats are supported their MIME type
will be added to the `Accept` header so that your server can return the
appropriate format.

`pub_date` if present must be formatted according to ISO 8601.

## Event: checking-for-update

Emitted when checking for update has started.

## Event: update-available

Emitted when there is an available update, the update would be downloaded
automatically.

## Event: update-not-available

Emitted when there is no available update.

## Event: update-downloaded

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateUrl` String
* `quitAndUpdate` Function

Emitted when update has been downloaded, calling `quitAndUpdate()` would restart
the application and install the update.

## autoUpdater.setFeedUrl(url)

* `url` String

Set the `url` and initialize the auto updater. The `url` could not be changed
once it is set.

## autoUpdater.checkForUpdates()

Ask the server whether there is an update, you have to call `setFeedUrl` before
using this API.
