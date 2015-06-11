# Using Selenium and WebDriver

From [ChromeDriver - WebDriver for Chrome][chrome-driver]:

> WebDriver is an open source tool for automated testing of web apps across many
> browsers. It provides capabilities for navigating to web pages, user input,
> JavaScript execution, and more. ChromeDriver is a standalone server which
> implements WebDriver's wire protocol for Chromium. It is being developed by
> members of the Chromium and WebDriver teams.

In Electron's [releases](https://github.com/atom/electron/releases) page you
can find archives of `chromedriver`, there is no difference between Electron's
distribution of `chromedriver` and upstream ones, so in order to use
`chromedriver` together with Electron, you will need some special setup.

Also notice that only minor version update releases (e.g. `vX.X.0` releases)
include `chromedriver` archives, because `chromedriver` doesn't change as
frequent as Electron itself.

## Setting up with WebDriverJs

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs) provided
a Node package for testing with web driver, we will use it as an example.

### 1. Start chrome driver

First you need to download the `chromedriver` binary, and run it:

```bash
$ ./chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

Remember the port number `9515`, which will be used later

### 2. Install WebDriverJS

```bash
$ npm install selenium-webdriver
```

### 3. Connect to chrome driver

The usage of `selenium-webdriver` with Electron is basically the same with
upstream, except that you have to manually specify how to connect chrome driver
and where to find Electron's binary:

```javascript
var webdriver = require('selenium-webdriver');

var driver = new webdriver.Builder()
  // The "9515" is the port opened by chrome driver.
  .usingServer('http://localhost:9515')
  .withCapabilities({chromeOptions: {
    // Here is the path to your Electron binary.
    binary: '/Path-to-Your-App.app/Contents/MacOS/Atom'}})
  .forBrowser('electron')
  .build();

driver.get('http://www.google.com');
driver.findElement(webdriver.By.name('q')).sendKeys('webdriver');
driver.findElement(webdriver.By.name('btnG')).click();
driver.wait(function() {
 return driver.getTitle().then(function(title) {
   return title === 'webdriver - Google Search';
 });
}, 1000);

driver.quit();
```

## Workflow

To test your application without rebuilding Electron, simply [place](https://github.com/atom/electron/blob/master/docs/tutorial/application-distribution.md) your app source into Electron's resource directory. 

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/


