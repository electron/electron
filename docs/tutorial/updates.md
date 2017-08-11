# Updating Applications

There are several ways to update an Electron application. The easiest and officially supported one is taking advantage of the built-in [Squirrel](https://github.com/Squirrel) framework and the [autoUpdater](../api/auto-updater.md) module that comes with it.

## Deploying an Update Server

To get started, you firstly need to deploy an update server (that's where the [autoUpdater](../api/auto-updater.md) module will download new updates from).

Depending on your needs, you can choose from one of these:

- [Hazel](https://github.com/zeit/hazel) – Pulls new releases from [GitHub Releases](https://help.github.com/articles/creating-releases/) and is **perfect for getting started**
- [Nuts](https://github.com/GitbookIO/nuts) – Also uses [GitHub Releases](https://help.github.com/articles/creating-releases/), but caches app updates on disk
- [electron-release-server](https://github.com/ArekSredzki/electron-release-server) – Provides you with a dashboard for handling releases

## Implementing Updates into Your App

Once you've deployed your update server, continue with importing the required modules in your code (please ensure that the code below will only be executed in production - you can use [electron-is-dev](https://github.com/sindresorhus/electron-is-dev) for that):

```js
const { app, autoUpdater } = require('electron')
```

Next, put together the URL of the update server:

```js
const server = <your-deployment-url>
const feed = `${server}/update/${process.platform}/${app.getVersion()}`
```

As the final step, tell [autoUpdater](../api/auto-updater.md) where to ask for updates:

```js
autoUpdater.setFeedURL(feed)
```

That's all. Once [built](../tutorial/application-distribution.md), your application will receive an update for each new [GitHub Release](https://help.github.com/articles/creating-releases/) that you create.

## Further Steps

Now that you've configured the basic update mechanism for your application, you need to ensure that the user will get notified when there's an update (this can be achieved using [events](../api/auto-updater.md#events)).

Also make sure that potential errors are [being handled](../api/auto-updater.md#event-error).
