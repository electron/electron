# อภิธานศัพท์ (Glossary)

เพจนี้จะบ่งบอกคำศัพท์ที่ถูกใช่บ่อยๆในการพัฒนา Electron

### ASAR

[ASAR (Atom Shell Archive Format)][asar] เป็น extension ของไฟล์
มันมีลักษณะความคล้ายครีง `tar` ที่รวมหลายไฟล์ลงไปในไฟล์เดียว 
Electron สามารถอ่านไฟล์ ASAR ได้เลยโดยที่ไม่ต้อง unpack

กำลังทำ: ไฟล์ประเภท ASAR นั้น สร้างมาเพื่อเพิ่มประสิทธิถาพบนระบบปฎิบัติการ Windows

### Brightray

[Brightray][brightray] เป็น static library ที่ทำให้ [libchromiumcontent] ใช้ได้ง่ายขึ้น
ในแอพพิเคชั่น มันถูกสร้างขึ้นมาเพื่อ Electron โดยเฉพาะ 
แต่ว่ามันสามารถเปิดใช้ในแอพที่ใช้ Chromium's render ซึ่งไม่ถูกสร้างจาก Electron ได้

Brightray เป็น dependency ของ Electron ที่ไม่ค่อยได้ใช้งานสำหรับผู้ใช้ Electron ทั่วไป

### DMG

DMG เป็นรูปแบบไฟล์ที่ใช้กับ Apple Disk Image

โดยส่วนมากไฟล์ DMG จะถูกใช้ในการแจกแจงตัวติดตั้งของแอพพิเคชั่น
คุณสามารถใช้ [electron-builder] ในการสร้างไฟล์ขึ้นมาได้

### IPC

IPC ย่อมาจาก Inter-Process Communication ซึ่ง Electron ใช้ IPC 
ในการที่จะส่งข้อความ JSON ระหว่าง [โปรเซสหลัก][main] และ [ตัวเรนเดอร์][renderer]

### libchromiumcontent

เป็น library รวมที่มี Chromium Content โมดูลและ dependencies ต่างๆเข้าไปด้วย
(อาทิเช่น Blink, [V8], ฯลฯ)

### โปรเซสหลัก

โปรเซสหลักในส่วนมากจะเป็นไฟล์ `main.js` ซึ่งเป็นจุดเริ่มต้นของทุกๆแอพของ Electron 
โปรเซสหลักเป็นตัวที่กำหนดการทำงานของแอพตั้งแต่ต้นจนจบ มันจะควบคุมองค์ประกอบของแอพเช่น
Menu, Menu Bar, Dock, Tray, ฯลฯ โปรเซสหลักนั้นจะรับผิดชอบการสร้างตัวเรนเดอร์ใหม่ในแอพ

Node API นั้นมาพร้อมกับ Electron

`package.json` จะสามารถใช้ระบุโปรเซสหลักของทุกๆแอพบน Electron ได้
นี้คือเหตุผลที่ `electron .` รู้ว่าไฟล์ไหนควรจะรันตอนเริ่มต้น

เพิ่มเติม: [process](#process), [renderer process](#renderer-process)

### MAS

MAS เป็นตัวย่อของ Mac App Store 
ในการที่จะส่งไฟล์ลง MAS นั้นโปรดดู [การแนะนำการส่ง Mac App Store](Mac App Store Submission Guide)

### native modules

Native โมดูล (อีกชื่อคือ [addons]) คือโมดูลที่เขียนด้วย C หรือ C++ 
ที่สามารถจะโหลดอยู่ใน Node.js หรือ Electron โดยการใช้ฟังค์ชั่น require() 
และใช้แบบเดียวกับโมดูล Node.js ธรรมดา

โดยส่วนมากมันถูกใช้เป็นอินเตอร์เฟสระหว่าง JavaScript และ Node.js ผ่านทาง C/C++

Native Node โมดูลจะได้รับการสนับสนุนจาก Electron
แต่ว่ามีความเป็นไปได้ว่า Electron นั้นจะใช้ V8 คนละเวอร์ชั่นกับ Node ที่มีอยู่ในระบบ
คุณจะต้องเป็นคนเลือก Electron's header ด้วยตนเอง

เพิ่มเติม: [วิธีการใช้ Native Node โมดูล][Using Native Node Modules]

## NSIS

NSIS ย่อมาจาก Nullsoft Scriptable Install System คือตัวติดตั้งที่ขับเคลื่อนด้วยสคริปท์
สำหรับระบบปฎิบัติการ Windows มันได้รับการจดลิขสิทธ์สำหรับใช้ทุกๆคน
มันถูกใช้แทนสินค้าเชิงพาณิชย์เช่น InstallShield 
[electron-builder] นั้นรองรับการสร้าง NSIS

### process

โปรเซสคือ instance ของโปรแกรมที่กำลังทำงานอยู่ 
Electron แอพนั้นใช้ [โปรเซสหลัก][main] และ [ตัวเรนเดอร์][renderer] หนึ่งตัวขึ้นไปที่รัน
หลายๆโปรเกรมพร้อมๆกัน

ใน Node.js และ Electron นั้น แต่ละโปรเซสที่กำลังรันอยู่จะมี `process` object
Object ตัวนี้เป็น global ที่ให้ข้อมูลและการควบคุมของตัวโปรเซสนั้นๆ

เพราะว่ามันเป็น global ดังนั้นจึงไม่จำเป็นที่จะเรียก `require()` เพื่อใช้งาน

เพิ่มเติม: [โปรเซสหลัก](#main-process), [ตัวเรนเดอร์](#renderer-process)

### renderer process

ตัวเรนเดอร์ คือ หน้าต่างบราวเซอร์ของแอพพิเคชั่นของคุณ
มันแตกต่างจากโปรเซสหลักโดยที่มันจะสามารถมีอยู่พร้อมกันหลายตัวได้
และแต่ละตัวนั้นใช้ โปรเซส ที่ต่างกันออกไป
นอกจากนั้นมันยังสามารถซ่อนได้อีกด้วย

ใน บราวเซอร์ ทั่วๆไป
เว็ปเพจจะได้รับการรันด้วยสภาพแวดล้อมจำกัดและจะไม่สามารถใช้ทรัพยากร native ได้

แต่ว่าสำหับผู้ใช้ Electron นั้นเราสามารถให้
เว็ปเพจมีการปฏิสัมพันธ์กับระบบปฎิบัติการได้โดยผ่านทาง API ของ Node.js

เพิ่มเติม: [โปรเซสหลัก](#main-process), [ตัวเรนเดอร์](#renderer-process)

### Squirrel

Squirrel เป็น open-source framework ที่ทำให้แอพ Electron นั้น 
อัพเดทได้อย่างอัตโนมัดเมื่อมีเวอร์ชั่นใหม่เข้ามา

เพิ่มเติม: [autoUpdater] API เพื่อการใช้งาน Squirrel เบื้องต้น

### userland

"Userland" แต่แรกทีมาจากสังคม unix ซึ่งหมายถึงโปรแกรมที่รันข้างนอก kernel ของระบบปฎิบัติการ
ภายหลังนี้ "userland" ได้รับความสนใจจากสังคม Node และ npm เพื่อใช้ในการอธิบายความแตกต่าง
ของ feature ที่พร้อมไช้งานใน "node core" กับ แพ็คเกจที่ได้รับการแจกจ่ายถายใน npm registry
ซึ่งมาจากเหล่าคนใช้ที่กว้างขวาง

Electron นั้นมีความคล้ายคลึงกับ Node ตรงที่มี API ที่ไม่เยอะแต่ว่ามีความสามารถเพียงพอ
ในการพัฒนา multi-platform แอพพิเคชั่น

ด้วยปรัชญาการออกแบบนี้เองที่ทำให้ Electron นั้นเป็นเครื่องมือที่มีความยืดหยุ่นโดยที่ไม่กำหนดให้ผู้ใช้
ใช้งานได้เพียงตามที่ออกแบบไว้

Userland ได้เปิดโอกาสให้ผู้ใช้สามารถสร้างและแบ่งปันเครื่องมือนอกเหนือจากอะไรก็ตามที่มีอยู่ใน "core"

### V8

V8 เป็น JavaScripe engine ของ Google ที่เป็น open source
มันถูกเขียนขึ้นด้วย C++ และถูกใช้ใน Google Chrome

Google Chrome เป็น open source บราวส์เซอร์ของ Google

V8 สามารถรันแยกเองต่างหากได้ หรือจะสามารถนำไปใช้กับแอพพิเคชั่น C++ อะไรก็ได้

### webview

`webview` เป็นแท็กที่ใช้ในการใส่ข้อมูลสำหรับคนใช้ทั่วไป (เช่นเว็ปภายนอก) ใน Electron แอพของคุณ
มันมีความคล้ายครึงกับ `iframe` แต่ว่าต่างกันโดยที่ webview รันโดยโปรเซสคนละตัว
มันไม่มี permission เหมือนกับเว็ปเพจของคุณและการมีปฏิสัมพันธ์ระหว่าง
แอพของคุงกับสิ่งที่ฝังอยู่จะเป็นไปโดยราบลื่นโดยที่ไม่ต้องซิ้งค์ (asynchronous)

ด้วยเหตุนี้เองทำให้ แอพของคุณปลอกภัยจากสิ่งที่ถูกฝัง


[addons]: https://nodejs.org/api/addons.html
[asar]: https://github.com/electron/asar
[autoUpdater]: api/auto-updater.md
[brightray]: https://github.com/electron/brightray
[electron-builder]: https://github.com/electron-userland/electron-builder
[libchromiumcontent]: #libchromiumcontent
[Mac App Store Submission Guide]: tutorials/mac-app-store-submission-guide.md
[main]: #main-process
[renderer]: #renderer-process
[Using Native Node Modules]: tutorial/using-native-node-modules.md
[userland]: #userland
[V8]: #v8
