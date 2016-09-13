
#Руководство по утверждению вашего приложения в App Store

Начиная с версии v0.34.0 Electron позволяет Вам сформировать данные для App Store к вашему приложению.
Данное руководство представляет собой пошаговую инструкцию по созданию данных файлов.

Помните, что когда Вы подаете свое приложение на рассмотрение в App Store Вы должны обладать аккаунтом разработчика,
который стоит денег.

## Как отправить свое приложение на рассмотрение в App Store

Последующие шаги подробно описывают, что и в какой последовательности Вам требуется сделать, но не гарантируют, что Ваше приложение будет рассмотрено Apple. Мы также рекомендуем Вам прочитать официальную документацию по оформлению своего приложения и информации к нему, чтобы пройти проверку в App Store.

## Получение сертификата

Перед тем, как отправить свое приложение Вы должны получить сертефикат, как это описано в этом [руководстве](https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps "Ссылка на руководство")

## Регистрируем свое приложение (подписываем)

После того, как Вы получили сертефикат, Вы можете упаковать свое прилоежние следуя правилам Application Distribution, 
а затем подписать свое приложение. Этот шаг является базовым, но подписывать свое приложение нам нужно всего лишь один раз.

Во-первых, нам нужно подготовить два файла:

`child.plist`:

```xml    
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
      <dict>
        <key>com.apple.security.app-sandbox</key>
        <true/>
        <key>com.apple.security.inherit</key>
        <true/>
      </dict>
    </plist>
```  

`parent.plist`:

```xml  
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
      <dict>
        <key>com.apple.security.app-sandbox</key>
        <true/>
        <key>com.apple.security.temporary-exception.sbpl</key>
        <string>(allow mach-lookup (global-name-regex #"^org.chromium.Chromium.rohitfork.[0-9]+$"))</string>
      </dict>
    </plist>
```

Затем подписываем свое приложение, с помощью специального сценария:

```bash
    #!/bin/bash

    # Имя вашего приложения.
    APP="YourApp"
    # Путь до вашейго приложения.
    APP_PATH="/path/to/YourApp.app"
    # Путь до вашего установочного пакета.
    RESULT_PATH="~/Desktop/$APP.pkg"
    # Имя сертификата которое вы хотите.
    APP_KEY="3rd Party Mac Developer Application: Company Name (APPIDENTITY)"
    INSTALLER_KEY="3rd Party Mac Developer Installer: Company Name (APPIDENTITY)"
    
    FRAMEWORKS_PATH="$APP_PATH/Contents/Frameworks"
    
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Electron Framework"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Libraries/libffmpeg.dylib"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Libraries/libnode.dylib"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper.app/Contents/MacOS/$APP Helper"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper.app/"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper EH.app/Contents/MacOS/$APP Helper EH"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper EH.app/"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper NP.app/Contents/MacOS/$APP Helper NP"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper NP.app/"
    codesign -s "$APP_KEY" -f --entitlements child.plist "$APP_PATH/Contents/MacOS/$APP"
    codesign -s "$APP_KEY" -f --entitlements parent.plist "$APP_PATH"
    
    productbuild --component "$APP_PATH" /Applications --sign "$INSTALLER_KEY" "$RESULT_PATH"
```

Если вы только начали разрабатывать под macOS, то мы советуем Вам прочитать [App SandBox](https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html "Ссылка для новичков в разработке приложений для macOS")

## Обновление приложения

После того, как Вы подписали свое приложение Вы сможете загрузить его в Itunes Connect для обработки, убедитесь, что Вы создали [запись](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html "ссылка на показ как создавать запись в Itunes Connect") перед отправкой.

## Объяснение использования 'temporary-exception'

Когда песочница Apple временно исключила ваше приложение, согласно [документации](https://developer.apple.com/library/mac/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/AppSandboxTemporaryExceptionEntitlements.html "Документация по исключениям") Вам нужно объяснить насколько это важное исключение:

>Примечание: если Вы временно исключаете свое приложение, обязательно прочитайте и выполните рекомендации по правам на исключение.
>которые предоставляются в Itunes Connect. Самое важное указать почему ваше приложение должно быть исключенно.

Вы можете объяснить, что ваше приложение построено на основе браузера Chromium, который использует Mach из-за его мультипроцессной архитектуры. Но есть еще вероятность, что ваше приложение не удалось проверить именно из-за этого.

## Отправка приложения на проверку

Следующие шаги описаны в официальной [документации](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html "Официальная статья по отправке приложения на проверку")

# Ограничения в Mac App Store

Для того чтобы удовлетворить всем просьбам SandBox App Store, некоторые из модулей были отключены:
- crashReporter
- autoUpdater

А также следующие проблемы были несколько изменены:
- Захват видео на некоторых машинах может не работать
- Некоторые специальные возможности могут не работать
- Приложения не будут в курсе изменения DNS

Также из-за использования SandBox App Store некоторые возможности могут быть не доступны или ограничены, подробнее о ограничениях 
Вы можете прочитать в [документации](https://developer.apple.com/app-sandboxing/ "Ссылка на ограничения в SandBox AppStore")

# Криптографические алгоритмы которые использует Electron

Смотря в какой стране и городе Вы находитесь, Apple может потребовать от Вас задокументировать алгоритмы криптографии которые Вы используете
и если потребуется, то попросит Вас предоставить копию регистрации вашего алгоритма шифрования.

Electron использует следующие алгоритмы шифрования:
- AES - NIST SP 800-38A, NIST SP 800-38D, RFC 3394
- HMAC - FIPS 198-1
- ECDSA - ANS X9.62–2005
- ECDH - ANS X9.63–2001
- HKDF - NIST SP 800-56C
- PBKDF2 - RFC 2898
- RSA - RFC 3447
- SHA - FIPS 180-4
- Blowfish - https://www.schneier.com/cryptography/blowfish/
- CAST - RFC 2144, RFC 2612
- DES - FIPS 46-3
- DH - RFC 2631
- DSA - ANSI X9.30
- EC - SEC 1
- IDEA - “On the Design and Security of Block Ciphers” book by X. Lai
- MD2 - RFC 1319
- MD4 - RFC 6150
- MD5 - RFC 1321
- MDC2 - ISO/IEC 10118-2
- RC2 - RFC 2268
- RC4 - RFC 4345
- RC5 - http://people.csail.mit.edu/rivest/Rivest-rc5rev.pdf
- RIPEMD - ISO/IEC 10118-3

Если Вы используете необычный алгоритм, то вот статья о том, как получить разрешение на использование собственного алгоритма шифрования в
рамках закона США - [статья](https://pupeno.com/2015/12/15/legally-submit-app-apples-app-store-uses-encryption-obtain-ern/ "Статья о том как получить разрешение на свой алгоритм шифрования")
