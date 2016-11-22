# Segurança, Capacidade Nativas, e sua Responsabilidade

Como desenvolvedores web, que normalmente aproveitamos a forte rede de segurança do navegador - os riscos associados com o código que escrevemos são relativamente pequenos. Nossos sites são concedidos com poderes limitados em uma sandbox, e confiamos que nossos usuários desfrutem de um navegador construído por uma grande equipe de engenheiros que é capaz de responder rapidamente a ameaças de segurança recém-descobertas.

Ao trabalhar com Electron, é importante compreender que Electron não é um navegador web. Ele permite que você construa aplicações desktop ricas em recursos com tecnologias web familiares, mas seu código exerce um poder muito maior. JavaScript pode acessar o sistema de arquivos, shell do usuário, e muito mais. Isto lhe permite construir aplicações nativas de alta qualidade, mas os riscos de segurança inerentes escala com os poderes adicionais concedidos ao seu código.

Com isso em mente, estar ciente de que a exibição de conteúdo arbitrário de fontes não confiáveis representa um risco de segurança grave que o Electron não se destina a tratar. Na verdade, os mais populares aplicativos do Electron (Atom, Slack, Visual Studio Code, etc) exibi conteúdo principalmente local (ou de confiança, conteúdo remoto seguro sem integração com Node) – se o seu aplicativo executa o código a partir de uma fonte on-line, é sua responsabilidade garantir que o código não é malicioso.

## Chromium Security Issues e Upgrades

Enquanto Electron se esforça para suportar novas versões do Chromium, logo que possível, os desenvolvedores devem estar cientes de que a atualização é um empreendimento importante - envolvendo dezenas de mãos ou mesmo centenas de arquivos. Dados os recursos e contribuições disponíveis hoje, Electron muitas vezes não vai estar na mais recente versão do Chromium, ficando para trás, quer por dias ou semanas.

Nós sentimos que nosso sistema atual de atualizar o componente do Chromium estabelece um equilíbrio adequado entre os recursos que temos disponíveis e as necessidades da maioria das aplicações contruídas em cima do framework. Nós definitivamente estamos interessados em saber mais sobre casos de uso específicos de pessoas que constroem coisas em cima do Electron. Pull requests e contribuições para apoiar este esforço são sempre muito bem-vindas.

## Igonorando o Conselho Acima

Um probema de segurança existe sempre que você receba um código a partir de um destino remoto e o executa locamente. Como exemplo, considere um site remoto que está sendo exibido dentro de uma janela do navegador. Se um atacante de alguma forma consegue mudar esse conteúdo (seja por atacar a fonte diretamente, ou por estar entre seu aplicativo e o destino real), eles serão capazes de executar código nativo na máquina do usuário.

> :warning: Sobre nenhuma circunstância você deve carregar e executar código remoto com integração com Node. Em vez disso, use somente arquivos locais (embalados juntamente com a sua aplicação) para executar código Node. Para exibir o conteúdo remoto, use a tag `webview` e certifique-se de desativar o `nodeIntegration`.

#### Checklist

Este não é á prova de balas, mas, ao menos, você deve tentar o seguinte:

* Somente exibir conteúdo seguro (https)
* Desativar a integração do Node em todos os renderizadores que exibem conteúdo remoto (utilizando `webPreferences`)
* Não desative `webSecurity`. Destivá-lo irá desativar a política de mesma origem.
* Definir um [`Content-Security-Policy`](http://www.html5rocks.com/en/tutorials/security/content-security-policy/), e usar regras restritivas (i.e. `script-src 'self'`)
* [Substituir e desativar `eval`](https://github.com/nylas/N1/blob/0abc5d5defcdb057120d726b271933425b75b415/static/index.js#L6-L8), que permite strings para ser executadas como código.
* Não defina `allowDisplayingInsecureContent` para true.
* Nao defina `allowRunningInsecureContent` para true.
* Não ative `experimentalFeatures` ou `experimentalCanvasFeatures` se você não sabe o que está fazendo.
* Não use `blinkFeatures` se você não sabe o que está fazendo.
* WebViews: Defina `nodeintegration` para false
* WebViews: Não use `disablewebsecurity`
* WebViews: Não use `allowpopups`
* WebViews: Não use `insertCSS` ou `executeJavaScript` com CSS/JS.

Mais uma vez, esta lista apenas minimiza o risco, não removê-la. Se o seu objetivo é o de apresentar um site, um navegador será uma opção mais segura.

## Buffer Global

Classe Node's [Buffer](https://nodejs.org/api/buffer.html) está atualmente disponível como global, mesmo quando `nodeIntegration` é definida como `false`. Você pode excluir este em seu aplicativo fazendo o seguinte no seu script de `preload`:

```js
delete global.Buffer
```

Excluí-lo pode quebrar módulos Node usado em seu script preload e aplicação já que muitas bibliotecas esperaram que ele seja global em vez de exigir-lo diretamente via:

```js
const {Buffer} = require('buffer')
```

O `Buffer` global pode ser removido em futuras versões principais do Electron.
