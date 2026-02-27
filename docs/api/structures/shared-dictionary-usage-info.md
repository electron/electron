# SharedDictionaryUsageInfo Object

* `frameOrigin` string - The origin of the frame where the request originates. It’s specific to the individual frame making the request and is defined by its scheme, host, and port. In practice, will look like a URL.
* `topFrameSite` string - The site of the top-level browsing context (the main frame or tab that contains the request). It’s less granular than `frameOrigin` and focuses on the broader "site" scope. In practice, will look like a URL.
* `totalSizeBytes` number - The amount of bytes stored for this shared dictionary information object in Chromium's internal storage (usually Sqlite).
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>My Browser</title>
<style>
body {
  margin: 0;
  font-family: Arial;
}

#toolbar {
  display: flex;
  background: #f1f3f4;
  padding: 10px;
}

#url {
  flex: 1;
  padding: 8px;
  border-radius: 20px;
  border: 1px solid #ccc;
}

button {
  margin-left: 5px;
  padding: 8px 12px;
}
</style>
</head>
<body>

<div id="toolbar">
  <input id="url" placeholder="Введите URL..." />
  <button onclick="loadPage()">Перейти</button>
</div>

<webview id="browser" style="width:100%; height:90vh;"></webview>

<script>
function loadPage() {
  let url = document.getElementById('url').value;
  if (!url.startsWith('http')) {
    url = 'https://' + url;
  }
  document.getElementById('browser').src = url;
}
</script>

</body>
</html>
