# Распространение приложения

Чтобы разпространять ваше приложение на Electron, папка с вашим приложением
должна называться `app` и находиться в папке ресурсов Electron (в macOS это
`Electron.app/Contents/Resources/`, в Linux и Windows - `resources/`),
вот так:

Для macOS:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Для Windows и Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Затем запустите `Electron.app` (или `electron` в Linux, `electron.exe` в Windows),
и Electron запустится как ваше приложение. Теперь папка `electron` и есть дистрибутив,
который Вы должны распространять пользователям.

## Упаковка вашего приложения в файл

Если Вы `не хотите` распространять исходные коды вашего проекта, Вы можете
упаковать его в архив [asar](https://github.com/atom/asar), чтобы не
показывать пользователям исходные коды.

Чтобы использовать `asar` для замены папки `app` на архив вам нужно
переименовать архив в `app.asar` и положить его в папку ресурсов Electron,
после чего Electron попробует считать ресурсы и запустить архив.


Для macOS:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

Для Windows и Linux:

```text
electron/resources/
└── app.asar
```

Больше деталей можно найти в [инстуркции по упаковке приложения](application-packaging.md).

## Ребрендирование скачанных исполняемых файлов

После того, как Вы подключили ваше приложение к Electron,
Вам наверняка захочется ребрендировать его перед распространением.

### Windows

Вы можете переименовать `electron.exe` как пожелаете и поменять иконку и прочую
информацию приложениями вроде [rcedit](https://github.com/atom/rcedit).

### macOS

Вы можете переименовать `Electron.app` как пожелаете, а также изменить
поля `CFBundleDisplayName`, `CFBundleIdentifier` и `CFBundleName` в следующих
файлах:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

Вы таже можете переименовать приложение-помощник, чтобы оно не показывало `Electron Helper`,
убедитесь, что Вы переименовали его исполняемый файл.

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
из исходных кодов. Чтобы сделать это Вам нужно изменить `atom.gyp` и полностью
пересобрать Electron.

### grunt-build-atom-shell

Проверка и пересборка кода Electron довольно сложная задача, так что мы
сделали файл-инструкцию для Grunt, который будет делать это автоматически:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Этот файл автоматически просмотрит изменения в `.gyp` файле, соберёт
Electron из исходных кодов и пересоберёт модули Node, чтобы всё подходило
под новое имя.

## Инструменты

Вы также можете использовать инструменты от третьих лиц,
которые сделают работу за вас:

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)
