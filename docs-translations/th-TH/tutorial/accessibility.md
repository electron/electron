# การเข้าถึง (Accessibility)

การที่จะทำให้แอพพิเคชั่นนั้นเข้าถึงได้เป็นเรื่องที่สำคัญมาก และ เรามีความสุขที่จะต้อนรับความสามารถใหม่ของเราสู่ [Devtron](http://electron.atom.io/devtron) และ [Spectron](http://electron.atom.io/spectron) ซึ่งได้ให้โอกาสผู้พัฒนาในการที่จะสร้างแอพพิเคชั่นที่ดีขึ้นเพื่อทุกๆคน

---

ความกังวลเกี่ยวกับการเข้าถึงของแอพพิเคชั่น Electron นั้นมีความคล้ายครึงกับความกังวลของเว็ปไซต์ เพราะว่าทั้งสองนั้นเป็น HTML ด้วยกัน ในขณะเดียวกันนั้น แต่ว่าในแอพ Electron คุณไม่สามารถใช้ทรัพยากรณ์ออนไลน์ได้เพราะว่าแอพของคุณนั้นไม่มี URL ที่สามารถเข้าถึงได้

ความสามารถใหม่ๆนี้นำอุปกรณ์การแก้ไขต่างๆเข้ามาใส่แอพ Electron ของคุณ คุณสามารถเลือกที่จะแก้ไขบททดสองของคุณได้ด้วย Spectron หรือว่าใช้มันใน DevTools ด้วย Devtron

กรุณาอ่านต่อเพื่อบทสรุปของอุปกรณ์หรือดู [เอกสารการเข้าถึง](http://electron.atom.io/docs/tutorial/accessibility) ของเราสำหรับข้อมูลเพิ่มเติม

### Spectron

ในการทดสอบเฟรมเวิร์ค Spectron นั้น
คุณจะใช้วิธีการแก้ไขทุกๆหน้าต่าง และ แท็ก `<webview>` ในแอพพิเคชั่นของคุณได้

ยกตัวอย่างเช่น:

```javascript
app.client.autidAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

คุณสามารถอ่านข้อมูลเพิ่มเติมสำหรับได้ที่ [เอกสาร Spectron](https://github.com/electron/spectron#accessibility-testing)

### Devtron

ใน Devtron นั้น จะมีแท็ปการเข้าถึง ซึ่งจะทำให้คุณสามารถจัดการเพจในแอพของคุณได้

![devtron screenshot](https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png)

ทั้งสองเครื่องมือใช้ [Accessibility Developer Tools](https://github.com/GoogleChrome/accessibility-developer-tools) ซึ่งเป็น library ที่สร้างขึ้นโดย Google เพื่อ Chrome

คุณสามารถศึกษาเพิ่มเติมเกี่ยวกับมันได้ที่ [รีโปนี้](https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules)

ถ้าคุณมีความรู้เกี่ยวกับอุปกรณือื่นๆที่สามารถใช้กับ Electron ได้ โปรดใส่มันเพิ่มใน [เอกสารการเข้าถึง](http://electron.atom.io/docs/tutorial/accessibility) ด้วยการขอดึงจาก Electron (pull request)
