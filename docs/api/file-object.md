# `File` Object

> Use the HTML5 `File` API to work natively with files on the filesystem.

The DOM's File interface provides abstraction around native files in order to
let users work on native files directly with the HTML5 file API. Electron has
added a `path` attribute to the `File` interface which exposes the file's real
path on filesystem.

Example of getting a real path from a dragged-onto-the-app file:

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
