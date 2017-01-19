# ลักษณะเอกสาร Electron

นี้คือกฎในการเขียนเอกสารประกอบ Electron

## หัวข้อ

* ทุกๆเพจจะมี `#` อยู่ข้างบนสุดของเอกสาร
* ทุกๆบทจะต้องมี `##` ในหัวข้อ
* ทุกๆบทย่อยจะเพิ่ม `#` ลงไปในหัวข้อตามความลึกของบทย่อย

ยกตัวอย่างเช่น `การเริ่มต้น`:

```markdown
# การเริ่มต้น

...

## โปรเซสหลัก

...

## โปรเซส render

...

## การรันแอพ

...

```

สำหรับการอ้างอิงของ API จะไม่ใช้กฎนี้

## กฎของ Markdown

* ใช้ `bash` แทน `cmd` ในการเขียน code blocks (เพราะตัวไฮไลท์ syntax)
* บรรทัดควรที่จะเริ่มต้นใหม่ที่ 80 คอลัมน์
* ไม่ควรใส่ลิสต์ที่มีมากกว่าสองชั้น (เพราะตัว render ของ markdown)
* ทุก `js` แลพ `javascript` code blocks จะไฮไลท์ด้วย
[standard-markdown](http://npm.im/standard-markdown).

## การใช้คำ

* ใช้ 'จะ' แทนที่ 'ควรจะ' ตอนอธิบายผลลัทธ์

## การอ้างอิงของ API

กฎดังต่อไปนี้จะใช้สำหรับเอกสาร API เท่านั้น

### หัวข้อเพจ

ทุกๆเพจจะใช้ชื่อของ object ที่ได้รับจาก `require('electron')` เป็นหัวข้อ อาทิเช่น `BrowserWindow`, `autoUpdater` และ `session`

ข้างล่างหัวข้อจะเป็นคำอธิบายหนึ่งบรรทัดเริ่มต้นด้วย `>`

```markdown
# session

> คำอธิบาย
```

### โมดูล methods และ events

สำหรับโมดูลที่ไม่ใช่คราสนั้น methods และ events ของมันจะต้องอยู่ในลิสต์ภายใต้บท `## Methods` และ `## Events`

ยกตัวอย่างเช่น `autoUpdater`:

```markdown
# autoUpdater

## Events

### Event: 'error'

## Methods

### `autoUpdater.setFeedURL(url[, requestHeaders])`
```

### คราส (Classes)

* คราส API หรือ คราสที่เป็นส่วนของโมดูลจะต้องอยู่ในบท `## Class: TheClassName`
* หนึ่งเพจสามารถมีหลายคราสได้
* Constructors จะต้องอยู่ใน `###`
* [Static Methods](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes/static) จะต้องอยู่ในบทของ `### Static Method`
* [Instance Methods](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes#Prototype_methods) จะต้องอยู่ในบทของ `### Instance Methods`
* ทุกๆ methods ที่มีค่า รีเทิร์น(return) จะต้องเริ่มต้นคำอธิบายด้วย "Returns `[TYPE]` - คำอธิบายค่ารีเทิร์น"
    * ถ้า method รีเทิร์น `Object` โครงสร้างจองมันจะอธิบายได้ด้วยการใช้ ','(โคล่อน) ตามด้วยบรรทัดใหม่ จากนั้นก็ ลิสต์ของคุณสมบัติที่ไม่เรียงกัน (เหมือนกับ parametres ของฟังค์ชั่น)
* Instance ของ Events จะต้องอยู่ในบทของ `### Instance Events`
* Instance ของ Properties จะต้องอยู่ในบทของ `### Instance Properties`
    * จะเริ่มต้นด้วย "A [Type ของ คุณสมบัติ]"

ยกตัวอย่างเช่น `Session` และ `Cookies` คราส:

```markdown
# session

## Methods

### session.fromPartition(partition)

## Properties

### session.defaultSession

## Class: Session

### Instance Events

#### Event: 'will-download'

### Instance Methods

#### `ses.getCacheSize(callback)`

### Instance Properties

#### `ses.cookies`

## Class: Cookies

### Instance Methods

#### `cookies.get(filter, callback)`
```

### Methods

บท methods จะอยู่ในรูปแบบดังนี้:

```markdown
### `objectName.methodName(required[, optional]))`

* `required` String - A parameter description.
* `optional` Integer (optional) - Another parameter description.

...
```

หัวข้อจะสามารถอยู่ในรูปของ `###` หรือ `####` (ตามว่าเป็น method ของโมดูลหรือของคราส)

สำหรับโมดูล `objectName` คือชื่อของโมดูล แต่ว่าสำหรับคราสนั้น ชื่อจะต้องเป็น instance ของคราสและจะต้องไม่ซ้ำกับชื่อโมดูล

ยกตัวอย่างเช่น method ของคราส `Session` ที่อยู่ใน `session` โมดูลจะต้องใช้ `ses` เหมือนกับ `objectName`.

หากมี arguments เพิ่มเตินนั้นจะได้รับการบันทึกภายใน `[]` รวมถึงคอมม่ถ้าหากว่า arguement นี้ตามด้วย argument เสริมอื่นๆ

```
required[, optional]
```

ข้างล่าง method จะเป็นข้อคาวมโดยละเอียดเกี่ยวกับ arguments อื่นๆ, ชนิค (Type) ของ argument จะโดนบันทีกตาม type ทั่วไป

* [`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)
* [`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)
* [`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)
* [`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
* [`Boolean`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)
* หรือ type พิเศษเช่น Electron [`WebContent`](api/web-contents.md)

หาก argument หรือ method มีเฉพาะเจาะจงกับ platform นั้น ให้ใช้ตัว italic (เอียง) ลิสต์ตาม datatype
โดยที่ตัวลิสต์นั้นจะจำกัดอยู่ที่ `macOS`, `Windows`, หรือ `Linux` เท่านั้น

```markdown
* `animate` Boolean (optional) _macOS_ _Windows_ - เล่นแอนนิเมชัั่น.
```

Arguments ชนิด `Array` จะต้องบอกว่า array นั้นจะใส่อะไรลงไปตามข้อมูลด้านล่าง

ข้อความอธิบาย arguments ของ ฟังค์ชั่น (function) จะต้องบอกเจาะจงถึงวิธีการใช้งานและชนิดของ parametres ที่ฟังค์ชั่นนั้นรับ

### อีเว้น (Events)

บทอีเว้นจะต้องอยู่ในรูปแบบดังนี้:

```markdown
### Event: 'wake-up'

Returns:

* `time` String

...
```

หัวข้อจะสามารถอยู่ในรูปของ `###` หรือ `####` (ตามว่าเป็นอีเว้นของโมดูลหรือของคราส)

แบบเอกสาร arguments ของอีเว้นจะใช้กฎเดียวกับ methods

### คุณสมบัติ (Properties)

บทคุณสมบัติจะต้องอยู่ในรูปแบบดังนี้:

```markdown
### session.defaultSession

...
```

หัวข้อจะสามารถอยู่ในรูปของ `###` หรือ `####` (ตามว่าเป็นคุณสมบัติของโมดูลหรือของคราส)

## การแปลเอกสาร

เอกสารของ Electron ที่ถูกแปลแล้ว จะอยู่ในโฟลเดอร์ `docs-translation`

ในการสร้างเซ็ตของภาษาใหม่ (เพิ่มในบางส่วน)

* สร้างที่อยู่ย่อยตามตัวย่อของภาษา
* แปลภาษา
* อัพเดท `README.md` ภายในเพื่อเป็นสารลิงค์ไฟล์ต่างๆที่แปลแล้ว
* เพิ่มลิ้งค์ลง [README](https://github.com/electron/electron#documentation-translations) หลักของ Electron

โน้ต: ไฟล์ที่อยู่ใน `docs-translations` จะมีแค่ที่แปลแล้วเท่านั้น ไฟล์ต้นตำรับภาษาอังกริษไม่ควรจะโดนก้อปปี้ลงไปด้วย
