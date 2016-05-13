# Распространение приложения

Чтобы разпространять ваше приложение на Electron, папка с вашим приложением
должна называться `app` и находиться в папке ресурсов Electron (на OS X это
`Electron.app/Contents/Resources/`, на Linux и Windows - `resources/`),
вот так:

На OS X:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

На Windows и Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Затем запустите `Electron.app` (или `electron` на Linux, `electron.exe` на Windows),
и Electron запустится как ваше приложение. Теперь папка `electron` и есть дистрибутив,
который вы должны распространять пользователям.

## Упаковка вашего приложения в файл

Если вы не хотите распространять исходные коды вашего проект, вы можете
упаковать его в архив [asar](https://github.com/atom/asar), чтобы не
показывать пользователям исходные коды.

Чтобы использовать `asar` для замены папки `app` на архив вам нужно
переименовать архив в `app.asar` и положить его в папку ресурсов Electron,
после чего Electron попробует считать ресурсы и запустить архив.


На OS X:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

На Windows и Linux:

```text
electron/resources/
└── app.asar
```

Больше деталей можна найти в [инстуркции по упаковке приложения](application-packaging.md).

## Ребрендирование скачанных исполняемых файлов
После того, как вы подключили ваше приложение к Electron,
вам наверняка захочеться ребрендировать его перед распространением.

### Windows

Вы можете переименовать `electron.exe` как пожелаете и поменять иконку и прочую
информацию приложениями вроде [rcedit](https://github.com/atom/rcedit).

### OS X

Вы можете переименовать `Electron.app` как пожелаете, а также изменить
поля `CFBundleDisplayName`, `CFBundleIdentifier` и `CFBundleName` в следующих
файлах:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

Вы таже можете переименовать приложение-помощник, чтобы оно не показывало `Electron Helper`,
убедитесь, что вы переименовали его исполняемый файл.

Структура переименованного приложения выглядит примерно так:

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

Вы можете переименовать исполняемый файл `electron` как пожелаете.

## Rebranding by Rebuilding Electron from Source

Вы также можете ребрендировать Electron изменив имя продукта и собрав его
из исходных кодов. Чтобы сделать это вам нужно изменить `atom.gyp` и полностью
пересобрать Electron.

### grunt-build-atom-shell

Проверка и пересборка кода Electron довольно сложная задача, так что мы
мы сделали файл-инструкцию для Grunt, который будет делать это автоматически:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Этот файл автоматически просмотрит изменения в `.gyp` фалле, соберёт
Electron из исходных кодов и пересоберёт модули Node, чтобы всё подходило
под новое имя.

## Инструменты

Вы также можете использовать инструменты оттретьих лиц,
которые сделают работу за вас:

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)
