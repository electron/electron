# `File` Object

> Use a API `File` do HTML5  para funcionar nativamente com os arquivos do sistema.


A interface dos arquivos DOM promovem uma abstração dos arquivos nativos, permitindo que os usuários trabalhem em arquivos nativos diretamente com a API file do HTML5. O Electron adicionou um atributo `path` à interface `File` que exibe o caminho do arquivo no sistema.

Exemplo de como obter o caminho de um arquivo arrastado para a aplicação:

```html
<div id="holder">
  Arraste e solte seu arquivo aqui
</div>

<script>
  const holder = document.getElementById('holder')
  holder.ondragover = () => {
    return false;
  }
  holder.ondragleave = holder.ondragend = () => {
    return false;
  }
  holder.ondrop = (e) => {
    e.preventDefault()
    for (let f of e.dataTransfer.files) {
      console.log('Arquivo(s) arrastados: ', f.path)
    }
    return false;
  }
</script>
```
