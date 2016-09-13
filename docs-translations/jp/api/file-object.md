# `File` object

> ファイルシステム上のファイルを扱うには、HTML5のFile APIを使用します。

DOMのファイルインターフェイスにより、ユーザーはHTML 5 ファイルAPIで直接、ネイティブファイルで作業できるように、ネイティブファイル周りの抽象化を提供します。Electronは、ファイルシステム上のファイルの実際のパスを公開する`File`インターフェイスの`path`属性を追加します。

アプリ上にドラッグしたファイルの実際のパスを取得する例:

```html
<div id="holder">
  Drag your file here
</div>

<script>
  const holder = document.getElementById('holder');
  holder.ondragover = () => {
    return false;
  };
  holder.ondragleave = holder.ondragend = () => {
    return false;
  };
  holder.ondrop = (e) => {
    e.preventDefault();
    const file = e.dataTransfer.files[0];
    console.log('File you dragged here is', file.path);
    return false;
  };
</script>
```
