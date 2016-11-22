# Acessibilidade

Fazendo aplicações acessíveis é importante e nós estamos felizes em apresentar uma nova funcionalidade para [Devtron](http://electron.atom.io/devtron) e [Spectron](http://electron.atom.io/spectron) que dá aos desenvolvedores a oportunidade de fazer as suas aplicações melhor para todos.

---

Preocupações de acessibilidade em aplicações Electron são semelhantes aos de websites, porque eles são ambos em última análise HTML. Com aplicativos Electron, no entanto, você não pode usar recursos on-line para auditorias de acessibilidade porque a sua aplicação não tem uma URL para apontar para o auditor.

Esses novos recursos trazem essas ferramentas de auditoria para a sua aplicação Electron. Você pode optar por adicionar auditorias aos seus testes com Spectron ou usá-los dentro do DevTools com Devtron. Leia a seguir para obter um resumo das ferramentas ou verifique nossa [documentação de acessibilidade](http://electron.atom.io/docs/tutorial/accessibility) para obter mais informações.

### Spectron

No framework de testes Spectron, agora você pode auditar cada janela e tag `<webview>` em seu aplicativo. Por exemplo:

```javascript
app.client.auditAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

Você pode ler mais sobre este recurso na [documentação do Spectron](https://github.com/electron/spectron#accessibility-testing).

### Devtron

Em Devtron há uma nova guia de acessibilidade que permitirá auditar uma página no seu aplicativo, classificar e filtrar os resultados.

![devtron screenshot](https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png)

Ambas as ferramentas estão usando a biblioteca [Accessibility Developer Tools](https://github.com/GoogleChrome/accessibility-developer-tools) construída pela Google for Chrome. Você pode aprender mais sobre as regras de auditoria da biblioteca de acessibilidade no [wiki do repositório](https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules).

Se você souber de outras ferramentas de acessibilidade para o Electron, adicione-as à [documentação de acessibilidade](http://electron.atom.io/docs/tutorial/accessibility) através de um pull request.
