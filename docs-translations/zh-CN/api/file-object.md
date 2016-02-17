# `文件`对象

为了让用户能够通过HTML5的file API直接操作本地文件，DOM的File接口提供了对本地文件的抽象。Electron在File接口中增加了一个path属性，它是文件在系统中的真实路径。

The DOM’s File interface provides abstraction around native files in order to let users work on native files directly with the HTML5 file API. Electron has added a path attribute to the File interface which exposes the file’s real path on filesystem.

---

获取拖动到APP中文件的真实路径的例子：

Example on getting a real path from a dragged-onto-the-app file:

```
<div id="holder">
  Drag your file here
</div>

<script>
  var holder = document.getElementById('holder');
  holder.ondragover = function () {
    return false;
  };
  holder.ondragleave = holder.ondragend = function () {
    return false;
  };
  holder.ondrop = function (e) {
    e.preventDefault();
    var file = e.dataTransfer.files[0];
    console.log('File you dragged here is', file.path);
    return false;
  };
</script>
```
