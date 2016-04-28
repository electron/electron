# `File` nesnesi

> Dosya ve dosya sistemlerinde HTML5 `File` nesnesini native olarak çalışır.

DOM Dosya arayüzü HTML5 dosya API'sini kullanarak kullanıcılara doğrudan native dosyalar üzerinde çalışmasına olanak sağlar. Electron'da `File` arayüzü için `path` özelliğini eklemiştir.

`dragged-onto-the-app`'dan tam dosya yolu alma örneği:

```html
<div id="holder">
  Dosyalarınızı buraya sürükleyin
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
    console.log('Sürükleyip buraktığınız dosyalar:', file.path);
    return false;
  };
</script>
```
