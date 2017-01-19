# คำถามที่ถูกถามบ่อยเกี่ยวกับ Electron (Electron FAQ)

## เมื่อไหร่ Electron จะอัพเกรดไปเวอร์ชั่นล่าสุดของ Chrome ?

โดยส่วยมาก Chrome เวอร์ชั่นใน Electron จะโดนอัพเกรดโดยประมาณหนี่งถึงสองอาทิตย์หลังจากมีเวอร์ชั่นใหม่ของ Chrome ที่เสถียร

Chrome เวอร์ชั่นที่เสถียรเท่านั้นที่จะถูกใช้ ถ้ามีการแก้บัคที่สำคัญในช่องทางเบต้าหรือพัฒนา เราจะนำมันเข้ามาใช้ด้วย

สำหรับข้อมูลเพิ่มเติม โปรดดูที่ [บทนำความปลอดภัย](tutorial/security.md)

## เมื่อไหร่ Electron จะอัพเกรดไปเวอร์ชั่นล่าสุดของ Node.js ?

เมื่อเวอร์ชั่นใหม่ของ Node.js ถูกปล่อยออกมา เราจะรอโดยประมาณหนึ่งเดือนก่อนที่จะอัพเกรด Node.js ที่อยู่ใน Electron เพื่อที่ว่าเราจะได้ลดความเสี่ยงถึงผลกระทบของบัคใน Node.js เวอร์ชั่นใหม่ซึ่งเกิดขึ้นบ่อยมาก

ความสามารถใหม่ของ Node.js โดยส่วนมากจะมากับการอัพเกรด V8 เนื่องจาก Electron นั้นใช้ V8 ที่มาพร้อมกับ Chrome browser อยู่แล้ว ทำให้ Electron มีความสามารถใหม่ของ JavaScript ที่มาพร้อมกับ Node.js เวอร์ชั่นใหม่อยู่แล้ว

## วิธีการแบ่งข้อมูลระหว่างเว็ปเพจ

ในการที่จะแบ่งข้อมูลนั้น (the renderer processes) วิธีการที่เรียบง่ายที่สุดคือการใช้ APIs ของ HTML5 ซี่งใช้ได้อยู่แล้วในเว็ปบราวเซอร์ ทางเลือกอื่นๆที่ดีคือ [Storage API][storage], [`localStorage`][local-storage],
[`sessionStorage`][session-storage], และ [IndexedDB][indexed-db].

หรือคุณจะสามารถใช้ระบบ IPC ซึ่งสำหรับ Electron มันจะเก็บ objects ในโปรเซสหลักในรูปของ global variable แล้วจึงเรียกมันจากตัว renderer ผ่านทาง `remote` ของ `electron` โมดูล

```javascript
// ในโปรเซสหลัก
global.sharedObject = {
  someProperty: 'default value'
}
```

```javascript
// ในเพจหนึ่ง
require('electron').remote.getGlobal('sharedObject').someProperty = 'new value'
```

```javascript
// ในเพจสอง
console.log(require('electron').remote.getGlobal('sharedObject').someProperty)
```

## หน้าต่างของแอพฉันหายไปหลังจากไม่กี่นาที

เหตุการณ์นี้เกิดขึ้นมื่อ variable ที่ใช้เก็บค่าหน้าต่างโดนหน่วยความจำเก็บกวาด

* [การจัดการหน่วยความจำ][memory-management]
* [ขอบเขตของตัวแปร][variable-scope]

วิธีการแก้แบบรวดเร็ว: เปลี่ยนตัวแปรให้เป็น global ด้วยการเปลี่ยนจากโค้ดนี้

```javascript
const {app, Tray} = require('electron')
app.on('ready', () => {
  const tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

เป็น :

```javascript
const {app, Tray} = require('electron')
let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

## ไม่สามารถใช้ jQuery/RequireJS/Meteor/AngularJS ใน Electron

เนื่องจากการรวบรวม Node.js เข้าไปใน Electron จึงทำให้เกิดการใส่อักขระเพิ่มเตืมลงไปใน DOM เช่น `module`, `export`, `require` 

มันทำให้เกิดปัญหากับ library อื่นๆที่ต้องการจะใช้อักขระตัวเดียวกัน

ในการแก้ปัญหานี้ คุณจะต้องปิด node ใน Electron:

```javascript
// ในโปรเซสหลัก
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
win.show()
```

แต่ถ้าคุณยังต้องการที่จะใช้ Node.js และ API ของ Electron คุณจะค้องเปลี่ยนชื่อของอักขระในเพจดังนี้ :

```html
<head>
<script>
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## `require('electron').xxx` is undefined.

ในตอนที่คุณใช้โมดูลที่มาพร้อมกับ Electron คุณอาจจะเจอปัญหาดังกล่าว:

```
> require('electron').webFrame.setZoomFactor(1.0);
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

มันเปิดมาจากคุณได้ลง [module npm `electron`][electron-module] ในเครื่องไม่ว่าจะเป็น locally หรือ globally ซึ่งมันจะทับโมดูลที่มาพร้อมกับ Electron

เพื่อตรวจสอบว่าคุณกำลังใช้โมดูลที่ถูกต้อง คุณสามารถที่จะส่ง command ที่จะปริ้น path ของ `electron` ได้:

```javascript
console.log(require.resolve('electron'))
```

แล้วก้เช็คว่าผลลัพท์อยู่ในรูปของ:

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

ถ้าผลลัพท์ที่ได้จากการส่ง command อยู่ในรูปแบบ `node_modules/electron/index.js` คุณจะต้องลบโมดูล `electron` ใน npm หรือไม่ก็เปลี่ยนชื่อมัน

```bash
npm uninstall electron
npm uninstall -g electron
```

ถ้าหากว่าคุณกำลังใช้โมดูลที่มาพร้อมกับ Electron แล้วยังเกิดข้อผิดผลาดดังกล่าว มีความเป็นไปได้สูงว่าคุณกำลังใช้โมดูลในโปรเซสที่ผิด

ยกตัวอย่างเช่น `electron.app` จะสามารถใช้ได้ในโปรเซสหลักเท่านั้น แต่ว่าในขณะเดียวกัน `electron.webFrame` นั้นใช้ได้ในโปรเซส renderer  เท่านั้น

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API
