# Electron约定

:+1::tada: 首先，感谢抽出时间做出贡献的每一个人。 :tada::+1:

这个项目将坚持贡献者盟约 [code of conduct](CODE_OF_CONDUCT.md).
我们希望贡献者能遵守贡献者盟约，如果有任何不能接受的行为被发现，请报告至atom@github.com(PS:请用英语)

下面是一组用于改进Electron的指南。
这些只是指导方针，而不是规则，做你认为对的事，并随时
在提出一个pull请求更改该文档。

## 提交 Issues

* 你可以在这里创建一个 issue [here](https://github.com/electron/electron/issues/new),
但在这样做之前请阅读以下注意事项，其中应包含尽可能多的细节。
如果可以能，请尽量包括:
  * Electron版本
  * 使用程序所使用的系统及系统版本
  * 如果适用，请包含：你做什么时发生了所提交的问题，以及你所期待的结果。
* 其他有助于解决你的 issue 的选项:
  * 截图或GIF动画
  * 终端中的错误堆栈
  * 执行 [cursory search](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  查看是否已经存在类似问题。

## 提交 Pull Requests

* 包括在你拉的请求截图和动画GIF只要有可能。
* 遵循的JavaScript，C ++和Python [coding style defined in docs](/docs/development/coding-style.md).
* 书写对应文档 [Markdown](https://daringfireball.net/projects/markdown).
  请参考 [Documentation Styleguide](/docs/styleguide.md).
* 使用简短明了的提交信息，请参考 [Commit Message Styleguide](#git-commit-messages).

## 文档样式

### 通用代码

* 以一个空行作为文件结尾。
* 请按以下顺序加载模块:
  * 内置 Node 模块 (参考 `path`)
  * 内置 Electron 模块 (参考 `ipc`, `app`)
  * 本地模块 (使用相对路径)
* 请按以下顺序排列类的属性与方法:
  * 类的方法和属性 (方法以 `@` 开始)
  * 实例的方法和属性
* 避免和平台相关的代码:
  * 使用 `path.join()` 来链接文件名
  * 当需要使用临时目录的时候使用 `os.tmpdir()` 而不是 `/tmp` 。
* 使用 `return` 进行明确返回时
  * 不使用 `return null`, `return undefined`, `null`, 或 `undefined`

### Git 提交信息(鉴于进行Git提交时需要英文书写，此处暂不翻译)

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally
* When only changing documentation, include `[ci skip]` in the commit description
* Consider starting the commit message with an applicable emoji:
  * :art: `:art:` when improving the format/structure of the code
  * :racehorse: `:racehorse:` when improving performance
  * :non-potable_water: `:non-potable_water:` when plugging memory leaks
  * :memo: `:memo:` when writing docs
  * :penguin: `:penguin:` when fixing something on Linux
  * :apple: `:apple:` when fixing something on macOS
  * :checkered_flag: `:checkered_flag:` when fixing something on Windows
  * :bug: `:bug:` when fixing a bug
  * :fire: `:fire:` when removing code or files
  * :green_heart: `:green_heart:` when fixing the CI build
  * :white_check_mark: `:white_check_mark:` when adding tests
  * :lock: `:lock:` when dealing with security
  * :arrow_up: `:arrow_up:` when upgrading dependencies
  * :arrow_down: `:arrow_down:` when downgrading dependencies
  * :shirt: `:shirt:` when removing linter warnings
