# Estrutura de Diretórios do Código-Fonte

O código-fonte do Electron é separado em algumas partes, seguindo principalmente as convenções de separação do chromium. 

Você pode se familiarizar com a [arquitetura de multiprocessamento ](http://dev.chromium.org/developers/design-documents/multi-process-architecture) do Chromium para entender melhor o código-fonte.


## Estrutura do Código-Fonte

```
Electron
├──atom - Código fonte do Electron.
|  ├── app - Código de inicialização.
|  ├── browser - A interface incluíndo a janela principal, UI, e todas as coisas do processo principal. Ele se comunica com o renderizador para gerenciar as páginas web.
|  |   ├── lib -  Código Javascript para inicializar o processo principal.
|  |   ├── ui - Implementação da UI para plataformas distintas.
|  |   |   ├── cocoa - Código-fonte específico do cocoa .
|  |   |   ├── gtk - Código-font específico do GTK+.
|  |   |   └── win - Código-fonte específico do Windows GUI.
|  |   ├── default_app - A página padrão é mostrada quando 
|  |   |   Electron inicializa sem fornecer um app.
|  |   ├── api - Implementação do processo principal das APIs
|  |   |   └── lib - Código Javascript, parte da implementação da API.
|  |   ├── net - Código relacionado a rede.
|  |   ├── mac - Código fonte em Object-c, específico para Mac.
|  |   └── resources - Icones, arquivos dependentes da plataforma, etc.
|  ├── renderer - Código que é executado no processo de renderização.
|  |   ├── lib -  Parte do código Javascript  de inicialização do renderizador.
|  |   └── api - Implementação das APIs para o processo de renderizaçãp.
|  |       └── lib - Código Javascript, parte da implementação da API.
|  └── common - Código que utiliza ambos os processos, o principal e o de rendezição,
|      ele inclui algumas funções utilitárias e códigos para integrar com ciclo de mensagens do node no ciclo de mensagens do Chromium.
|      ├── lib - Código Javascript comum para a inicialização.
|      └── api - A implementação de APIs comuns e fundamentação dos
|          módulos integrados com Electron's.
|          └── lib - Código Javascript, parte da implementação da API.
├── chromium_src - Código-fonte copiado do Chromium.
├── docs - Documentação.
├── spec - Testes Automáticos.
├── atom.gyp - Regras de compilação do Electron.
└── common.gypi - Configuração específica do compilador e regras de construção para outros componentes
    como `node` e `breakpad`.
```

## Estrutura de Outros Diretórios.

* **script** - Scripts utilizado para fins de desenvolvimento como building, packaging, testes, etc.
* **tools** - Scripts auxiliares, utilizados pelos arquivos gyp, ao contrário do`script`, os scripts colocados aqui nunca devem ser invocados diretamente pelos usuários.
* **vendor** - Dependências de código-fonte de terceiros, nós não utilizamos  `third_party` como nome porque ele poderia ser confundido com o diretório homônimo existente no código-fonte do Chromium.
* **node_modules** - Módulos de terceiros em node usados para compilação
* **out** - Diretório temporário saída do `ninja`.
* **dist** - Diretório temporário do `script/create-dist.py` ao criar uma distribuição
* **external_binaries** - Binários baixados de Frameworks de terceiros que não suportam a compilação com `gyp`.
