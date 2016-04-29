# Utilizando Selenium y WebDriver

De [ChromeDriver - WebDriver for Chrome][chrome-driver]:

> WebDriver es una herramienta de código abierto para automatizar el testing de aplicaciones web
> en varios navegadores. WebDriver provee funciones de navegación, entrada de usuario,
> ejecución de JavaScript, y más. ChromeDriver es un servidor standalone que implementa
> el protocolo de WebDriver para Chromium. Se encuentra en desarrollo por los miembros de 
> Chromium y WebDriver.

En la página de [lanzamientos](https://github.com/electron/electron/releases) de Electron encontrarás paquetes de `chromedriver`.

## Ajustando parámetros con WebDriverJs

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs) provee
un paquete Node para realizar testing con web driver, lo usaremos como ejemplo.

### 1. Inicia chrome driver

Primero necesitas descargar el binario `chromedriver` y ejecutarlo:

```bash
$ ./chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

Recuerda el puerto `9515`, lo utilizaremos después.

### 2. Instala WebDriverJS

```bash
$ npm install selenium-webdriver
```

### 3. Conecta chrome driver

El uso de `selenium-webdriver` junto con Electron es básicamente el mismo que el original,
excepto que necesitas especificar manualmente cómo se conectará el chrome driver
y dónde encontrará el binario de Electron:

```javascript
var webdriver = require('selenium-webdriver');

var driver = new webdriver.Builder()
  // El puerto "9515" es que abre chrome driver.
  .usingServer('http://localhost:9515')
  .withCapabilities({chromeOptions: {
    // Aquí especificamos la ruta a Electron
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

Para probar tu aplicación sin recompilar Electron, simplemente [copia](https://github.com/electron/electron/blob/master/docs/tutorial/application-distribution.md) las fuentes de tu aplicación en el directorio de recursos de Electron.

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/


