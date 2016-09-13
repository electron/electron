# process

在 Electron 裡的 `process` 物件具有以下幾個與 upstream node 的不同點:

* `process.type` String - Process 的型態，可以是 `browser` (i.e. 主行程) 或 `renderer`.
* `process.versions.electron` String - Electron 的版本
* `process.versions.chrome` String - Chromium 的版本
* `process.resourcesPath` String - JavaScript 源碼的路徑

# 方法 (Methods)

`process` 物件具有以下的方法:

### `process.hang`

會導致目前行程的主執行緒停住

## process.setFdLimit(maxDescriptors) _macOS_ _Linux_

* `maxDescriptors` Integer

設置文件描述符 (file descriptor) soft limit `maxDescriptors` 或 OS hard
limit ，以較低者為準當目前的行程。
