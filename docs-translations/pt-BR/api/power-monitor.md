# powerMonitor

> Monitorar as mudanças de estado de energia.

Você só pode usá-lo no processo principal. Você não deve usar este módulo até que o evento `ready` do modulo `app` seja emitido.

Por exemplo:

```javascript
app.on('ready', () => {
  require('electron').powerMonitor.on('suspend', () => {
    console.log('O sistema está indo dormir')
  })
})
```

## Eventos

O módulo `power-monitor` emite os seguintes eventos:

### Evento: 'suspend'

Emitido quando o sistema está a suspender.

### Evento: 'resume'

Emitido quando o sistema está a retomar.

### Evento: 'on-ac' _Windows_

Emitido quando o sistema muda para energia AC.

### Evento: 'on-battery' _Windows_

Emitido quando o sistema muda para a energia da bateria.
