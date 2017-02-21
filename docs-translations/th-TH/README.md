กรุณาตรวจสอบว่าคุณกำลังใช้คู่มือที่ตรงกับเวอร์ชั่นของ Electon ของคุณด้วย ตัวเลขเวอร์ชั่นจะมีบอกใน URL ของหน้าเพจ ถ้าไม่มีหมายความว่าคุณอาจจะใช้เอกสารของ development branch ที่ API อาจจะมีการเปลี่ยนแปลง ซึ่งไม่สามารถใช้ร่วมกับ Electron เวอร์ชั่นที่คุณใช้อยู่ได้ เพื่อที่จะดูเอกสารเวอร์ชั่นเก่า [คุณสามารถที่จะดูแท็ก](https://github.com/electron/electron/tree/v1.4.0) ใน GitHub โดยการที่คลิกที่ "เรียกดูตามกิ่ง/แท็ก" แล้วเลือกแท็กที่ตรงกับเวอร์ชั่นของคุณ

## คำถามที่ถูกถามบ่อย

รวบรวมคำถามที่ถูกถามบ่อย กรุณาอ่านก่อนเปิด issue:

* [คำถามที่ถูกถามบ่อยเกี่ยวกับ Electron](faq.md)

## คู่มือ

* [คำศัพท์เฉพาะ](glossary.md)
* [แพลตฟอร์มที่รองรับ](tutorial/supported-platforms.md)
* [ความปลอดภัย](tutorial/security.md)
* [การเผยแพร่แอปพลิเคชัน](tutorial/application-distribution.md)
* [แนวทางการส่งแอปเข้า Mac App Store](tutorial/mac-app-store-submission-guide.md)
* [คู่มือ Windows Store](tutorial/mac-app-store-submission-guide.md)
* [การบรรจุแอปพลิเคชัน](tutorial/application-packaging.md)
* [การใช้โมดูลของ Node](tutorial/using-native-node-modules.md)
* [การหาข้อผิดพลาดในกระบวนการหลัก](tutorial/debugging-main-process.md)
* [การใช้งาน Selenium และ WebDriver](tutorial/using-selenium-and-webdriver.md)
* [ส่วนเสริมของ DevTools](tutorial/devtools-extension.md)
* [การใช้งานส่วนเสริม Pepper Flash](tutorial/using-pepper-flash-plugin.md)
* [การใช้งานส่วนเสริม Widevine CDM Plugin](tutorial/using-pepper-flash-plugin.md)
* [การทดสอบบน CI (Travis, Jenkins)](tutorial/testing-on-headless-ci.md)
* [การเลนเดอร์นอกหน้าต่าง] (tutorial/offscreen-rendering.md)

## แนะนำ

* [เริ่มต้นอย่างคราวๆ](tutorial/quick-start.md)
* [การร่วมกันของสภาพแวดล้อมบนเดสทอป](tutorial/desktop-environment-integration.md)
* [การตรวจจับเหตุการณ์ออนไลน์หรือออฟไลน์](tutorial/online-offline-events.md)
* [REPL](tutorial/repl.md)

## แหล่งอ้างอิงของ API

* [สรุปความ](api/synopsis.md)
* [โปรเซสออบเจค](api/process.md)
* [คำสั่งสำหรับเปลี่ยนแปลงค่าของ Chrome ที่รองรับ](api/chrome-command-line-switches.md)
* [Variables สภาพแวดล้อม](api/environment-variables.md)

### การปรับแต่ง DOM:

* [วัตถุ `File`](api/file-object.md)
* [แท็ก `<webview>`](api/webview-tag.md)
* [ฟังก์ชัน `window.open`](api/window-open.md)

### โมดูลสำหรับกระบวนการหลัก :

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* [BrowserWindow](api/browser-window.md)
* [contentTracing](api/content-tracing.md)
* [dialog](api/dialog.md)
* [globalShortcut](api/global-shortcut.md)
* [ipcMain](api/ipc-main.md)
* [Menu](api/menu.md)
* [MenuItem](api/menu-item.md)
* [powerMonitor](api/power-monitor.md)
* [powerSaveBlocker](api/power-save-blocker.md)
* [protocol](api/protocol.md)
* [session](api/session.md)
* [systemPreferences](api/system-preferences.md)
* [Tray](api/tray.md)
* [webContents](api/web-contents.md)

### โมดูลสำหรับกระบวนการ Renderer (เว็บเพจ):

* [desktopCapturer](api/desktop-capturer.md)
* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

### โมดูลสำหรับทั้งสองกระบวนการ:

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## การพัฒนา 

* [ลักษณะการเขียนโค้ด](development/coding-style.md)
* [การใช้ clang-format สำหรับโค้ด C++](development/clang-format.md)
* [โครงสร้างไดเรคทอรี่ของซอร์สโค้ด](development/source-code-directory-structure.md)
* [ความแตกต่างทางเทคนิคจาก NW.js (หรือ node-webkit)](development/atom-shell-vs-node-webkit.md)
* [ภาพรวมการสร้างระบบ](development/build-system-overview.md)
* [ขั้นตอนการสร้าง (macOS)](development/build-instructions-osx.md)
* [ขั้นตอนการสร้าง (Windows)](development/build-instructions-windows.md)
* [ขั้นตอนการสร้าง (Linux)](development/build-instructions-linux.md)
* [ขั้นตอนการแก้บัค (macOS)](development/debugging-instructions-macos.md)
* [ขั้นตอนการแก้บัค (Windows)](development/debug-instructions-windows.md)
* [การติดตั้งเซิร์ฟเวอร์ Symbol Server ใน debugger](development/setting-up-symbol-server.md)
* [ลักษณะการแก้เอกสาร](styleguide.md)
