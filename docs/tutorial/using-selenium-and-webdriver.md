# Using Selenium and WebDriver

From [ChromeDriver - WebDriver for Chrome][chrome-driver]:

> WebDriver is an open source tool for automated testing of web apps across many
> browsers. It provides capabilities for navigating to web pages, user input,
> JavaScript execution, and more. ChromeDriver is a standalone server which
> implements WebDriver's wire protocol for Chromium. It is being developed by
> members of the Chromium and WebDriver teams.

## Setting up Spectron

[Spectron][spectron] is the officially supported ChromeDriver testing framework
for Electron. It is built on top of [WebdriverIO](https://webdriver.io/) and
has helpers to access Electron APIs in your tests and bundles ChromeDriver.

```sh
$ npm install --save-dev spectron
```

```javascript
// A simple test to verify a visible window is opened with a title
const Application = require('spectron').Application
const assert = require('assert')

const myApp = new Application({
  path: '/Applications/MyApp.app/Contents/MacOS/MyApp'
})

const verifyWindowIsVisibleWithTitle = async (app) => {
  await app.start()
  try {
    // Check if the window is visible
    const isVisible = await app.browserWindow.isVisible()
    // Verify the window is visible
    assert.strictEqual(isVisible, true)
    // Get the window's title
    const title = await app.client.getTitle()
    // Verify the window's title
    assert.strictEqual(title, 'My App')
  } catch (error) {
    // Log any failures
    console.error('Test failed', error.message)
  }
  // Stop the application
  await app.stop()
}

verifyWindowIsVisibleWithTitle(myApp)
```

## Setting up with WebDriverJs

[WebDriverJs](https://www.selenium.dev/selenium/docs/api/javascript/index.html) provides
a Node package for testing with web driver, we will use it as an example.

### 1. Start ChromeDriver

First you need to download the `chromedriver` binary, and run it:

```sh
$ npm install electron-chromedriver
$ ./node_modules/.bin/chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

Remember the port number `9515`, which will be used later

### 2. Install WebDriverJS

```sh
$ npm install selenium-webdriver
```

### 3. Connect to ChromeDriver

The usage of `selenium-webdriver` with Electron is the same with
upstream, except that you have to manually specify how to connect
chrome driver and where to find Electron's binary:

```javascript
const webdriver = require('selenium-webdriver')

const driver = new webdriver.Builder()
  // The "9515" is the port opened by chrome driver.
  .usingServer('http://localhost:9515')
  .withCapabilities({
    chromeOptions: {
      // Here is the path to your Electron binary.
      binary: '/Path-to-Your-App.app/Contents/MacOS/Electron'
    }
  })
  .forBrowser('electron')
  .build()

driver.get('http://www.google.com')
driver.findElement(webdriver.By.name('q')).sendKeys('webdriver')
driver.findElement(webdriver.By.name('btnG')).click()
driver.wait(() => {
  return driver.getTitle().then((title) => {
    return title === 'webdriver - Google Search'
  })
}, 1000)

driver.quit()
```

## Setting up with WebdriverIO

[WebdriverIO](https://webdriver.io/) provides a Node package for testing with web
driver.

### 1. Start ChromeDriver

First you need to download the `chromedriver` binary, and run it:

```sh
$ npm install electron-chromedriver
$ ./node_modules/.bin/chromedriver --url-base=wd/hub --port=9515
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

Remember the port number `9515`, which will be used later

### 2. Install WebdriverIO

```sh
$ npm install webdriverio
```

### 3. Connect to chrome driver

```javascript
const webdriverio = require('webdriverio')
const options = {
  host: 'localhost', // Use localhost as chrome driver server
  port: 9515, // "9515" is the port opened by chrome driver.
  desiredCapabilities: {
    browserName: 'chrome',
    'goog:chromeOptions': {
      binary: '/Path-to-Your-App/electron', // Path to your Electron binary.
      args: [/* cli arguments */] // Optional, perhaps 'app=' + /path/to/your/app/
    }
  }
}

const client = webdriverio.remote(options)

client
  .init()
  .url('http://google.com')
  .setValue('#q', 'webdriverio')
  .click('#btnG')
  .getTitle().then((title) => {
    console.log('Title was: ' + title)
  })
  .end()
```

## Workflow

To test your application without rebuilding Electron,
[place](https://github.com/electron/electron/blob/master/docs/tutorial/application-distribution.md)
your app source into Electron's resource directory.

Alternatively, pass an argument to run with your Electron binary that points to
your app's folder. This eliminates the need to copy-paste your app into
Electron's resource directory.

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/
[spectron]: https://electronjs.org/spectron
