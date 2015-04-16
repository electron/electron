# `File` object

The DOM's File interface provides abstraction around native files, in order to
let users work on native files directly with HTML5 file API, Electron has
added a `path` attribute to `File` interface which exposes the file's real path
on filesystem.

Example on getting real path of a dragged file:

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
