# Distribuição de aplicações

Para distribuir sua aplicação com o Electron, você deve nomear o diretório que contém sua aplicação como 
`app` e dentro deste diretório colocar os recursos que você está utilizando (no OSX 
`Electron.app/Contents/Resources/`,
no Linux e no Windows é em `resources/`):

No OSX:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

No Windows e Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Logo após execute `Electron.app` (ou `electron` no Linux e `electron.exe` no Windows),
e o Electron iniciaria a aplicação. O diretório `electron` será utilizado para criar a distribuição para 
usuários finais.

## Empacotando sua aplicação em um arquivo.

Além de copiar todos os seus arquivos fontes para a distribuição, você também pode
empacotar seu aplicativo em um arquivo [asar](https://github.com/atom/asar) para evitar
de expor seu código fonte aos usuários finais.

Para usar um arquivo `asar` ao invés da pasta `app` você precisa mudar o nome do
arquivo para `app.asar` e colocá-lo sob o diretório de recursos do Electron como
mostrado abaixo, então o Electron vai ler o arquivo e iniciar a aplicação a partir dele.

No OSX:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

No Windows e Linux:

```text
electron/resources/
└── app.asar
```

Mais detalhes podem ser encontrados em [Empacotamento da aplicação](../../../docs/tutorial/application-packaging.md).

## Renomeando a marca Electron na sua distribuição

Depois de empacotar seu aplicativo Electron, você vai querer renomear a marca Electron
antes de distribuí-lo aos usuários.

### Janelas

Você pode renomear `electron.exe` para o nome que desejar e editar o seu ícone e outras
informações com ferramentas como [rcedit](https://github.com/atom/rcedit) ou
[ResEdit](http://www.resedit.net).

### OS X

Você pode renomear `Electron.app` para o nome que desejar e também pode mudar o nome
do `CFBundleDisplayName`, `CFBundleIdentifier` e os campos em `CFBundleName`
nos seguinte arquivos:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/frameworks/Electron Helper.app/Contents/Info.plist`

Você também pode renomear o arquivo de ajuda para evitar a exibição de `Electron Helper` no
Monitor de Atividades, mas certifique-se de também renomear o arquivo de ajuda no executável do
aplicativo.

A estrutura de uma aplicação renomada seria assim:

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

Você pode renomear o executável `electron` para o nome que desejar.

## Renomeando a marca Electron do código fonte.

Também é possível fazer renomear a marca Electron do código fonte, alterando o nome do produto e
reconstruí-lo a partir da fonte, para fazer isso você precisa modificar o arquivo `atom.gyp`.

### grunt-build-atom-shell

A modificação do código fonte do Electron para ganhar a sua marca pode ser muito complexa, por isso,
uma tarefa para o Grunt foi criado e irá cuidar desta tarefa automaticamente para você:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Esta tarefa irá automaticamente editar o arquivo `.gyp`, compilar o código
e reconstruir os módulos nativos da aplicação para utilizar o novo nome.