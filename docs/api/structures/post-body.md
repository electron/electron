# PostBody Object

* `data` ([UploadRawData](upload-raw-data.md) | [UploadFile](upload-file.md))[] - The post data to be sent to the
  new window.
* `contentType` String - The `content-type` header used for the data. One of
  `application/x-www-form-urlencoded` or `multipart/form-data`. Corresponds to
  the `enctype` attribute of the submitted HTML form.
* `boundary` String (optional) - The boundary used to separate multiple parts of
  the message. Only valid when `contentType` is `multipart/form-data`.

Note that keys starting with `--` are not currently supported. For example, this will errantly submit as `multipart/form-data` when `nativeWindowOpen` is set to `false` in webPreferences:

```html
<form
  target="_blank"
  method="POST"
  enctype="application/x-www-form-urlencoded"
  action="https://postman-echo.com/post"
>
  <input type="text" name="--theKey">
  <input type="submit">
</form>
```
