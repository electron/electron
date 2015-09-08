# `File` object

DOM's File 介面提供一個將本地文件抽象化，並可以讓使用者對本地文件直接使用 HTML5 檔案 API
Electron 可以添加一個 `path` 屬性至 `File` 接口進而顯示檔案在檔案系統內的真實路徑。

範例，獲得一個檔案之真實路徑，將檔案拖拉至應用程式 (dragged-onto-the-app):

```html
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
