# `File` 对象

> 在文件系统上使用 HTML5 `File` API 与本地文件交互。

为了让用户能够通过 HTML5 的 file API 直接操作本地文件，DOM 的 File 接口提供了对本地文件的抽象。Electron 在 File 接口中增加了一个 path 属性，它是文件在系统中的真实路径。

获取拖动到 APP 中文件的真实路径的例子：

```html
<div id="holder">
  Drag your file here
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
      console.log('File(s) you dragged here: ', f.path)
    }
    return false;
  }
</script>
```
