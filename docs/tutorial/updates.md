# Updating Applications

There are several ways to update an Electron application. The easiest and 
officially supported one is taking advantage of the built-in 
[Squirrel](https://github.com/Squirrel) framework and 
Electron's [autoUpdater](../api/auto-updater.md) module.

## Deploying an update server

To get started, you first need to deploy a server that the 
[autoUpdater](../api/auto-updater.md) module will download new updates from.

Depending on your needs, you can choose from one of these:

- [Hazel](https://github.com/zeit/hazel) – Pulls new releases from 
[GitHub Releases](https://help.github.com/articles/creating-releases/) and can 
be deployed for free on [Now](https://zeit.co/now).
- [Nuts](https://github.com/GitbookIO/nuts) – Also uses 
[GitHub Releases](https://help.github.com/articles/creating-releases/), 
but caches app updates on disk and supports private repositories.
- [electron-release-server](https://github.com/ArekSredzki/electron-release-server) – 
Provides a dashboard for handling releases

## Implementing updates in your app

Once you've deployed your update server, continue with importing the required 
modules in your code. The following code might vary for different server 
software, but it works like described when using 
[Hazel](https://github.com/zeit/hazel).

**Important:** Please ensure that the code below will only be executed in 
your packaged app. You can use 
[electron-is-dev](https://github.com/sindresorhus/electron-is-dev) to check for 
the environment.

```js
const { app, autoUpdater } = require('electron')
```

Next, construct the URL of the update server and tell 
[autoUpdater](../api/auto-updater.md) about it:

```js
const server = 'https://your-deployment-url.com'
const feed = `${server}/update/${process.platform}/${app.getVersion()}`

autoUpdater.setFeedURL(feed)
```

As the final step, check for updates (the example below will check every minute):

```js
setInterval(() => {
  autoUpdater.checkForUpdates()
}, 60000)
```

That's all. Once [built](../tutorial/application-distribution.md), your 
application will receive an update for each new 
[GitHub Release](https://help.github.com/articles/creating-releases/) that you 
create.

## Further steps

Now that you've configured the basic update mechanism for your application, you 
need to ensure that the user will get notified when there's an update. This
can be achieved using [events](../api/auto-updater.md#events):

```js
autoUpdater.on('update-downloaded', (event, releaseNotes, releaseName) => {
  // Show a notification banner to the user that allows triggering the update
})
```

Also make sure that errors are 
[being handled](../api/auto-updater.md#event-error). Here's an example
for logging them to `stderr`:

```js
autoUpdater.on('error', message => {
  console.error('There was a problem updating the application')
  console.error(message)
})
```
