[![Electron Logo](http://electron.atom.io/images/electron-logo.svg)](http://electron.atom.io/)

[![Travis Build Status](https://travis-ci.org/electron/electron.svg?branch=master)](https://travis-ci.org/electron/electron)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/kvxe4byi7jcxbe26/branch/master?svg=true)](https://ci.appveyor.com/project/Atom/electron)
[![devDependency Status](https://david-dm.org/electron/electron/dev-status.svg)](https://david-dm.org/electron/electron#info=devDependencies)
[![Join the Electron Community on Slack](http://atom-slack.herokuapp.com/badge.svg)](http://atom-slack.herokuapp.com/)

Electron框架，可用JavaScript, HTML 及 CSS 编写桌面程序。
它基于[Node.js](https://nodejs.org/)及[Chromium](http://www.chromium.org)
[Atom editor](https://github.com/atom/atom)以及[apps](http://electron.atom.io/apps)就是使用Electron编写的。

请关注Twitter [@ElectronJS](https://twitter.com/electronjs) 的重要公共。

这个项目将坚持贡献者盟约 [code of conduct](CODE_OF_CONDUCT.md).
我们希望贡献者能遵守贡献者盟约，如果有任何不能接受的行为被发现，请报告至electron@github.com(PS:请用英语)

## 下载

预编译的二进制版本及symbols调试版本可以在[releases](https://github.com/electron/electron/releases)找到，
其中包括Linux,Windows和macOS版本

你可以使用[`npm`](https://docs.npmjs.com/)来安装预编译版本的electron
You can also use  to install prebuilt electron
二进制文件:

```sh
# 全局安装 `electron`
npm install electron -g

# 安装为开发依赖
npm install electron --save-dev

# 淘宝源全局安装(PS:大陆到Electron源的下载速度极不稳定，无法下载成功时可用)
ELECTRON_MIRROR=http://npm.taobao.org/mirrors/electron/ npm install -g electron-prebuilt

# 淘宝源开发依赖安装(PS:大陆到Electron源的下载速度极不稳定，无法下载成功时可用)
ELECTRON_MIRROR=http://npm.taobao.org/mirrors/electron/ npm install electron-prebuilt --save-dev
```

### 其他源

- [China](https://npm.taobao.org/mirrors/electron)

## 文档

开发指南及API文档位于
[docs](https://github.com/electron/electron/tree/master/docs)
它也包括如何编译和改进Electron

## 翻译版文档

- [葡萄牙语－巴西](https://github.com/electron/electron/tree/master/docs-translations/pt-BR)
- [韩语](https://github.com/electron/electron/tree/master/docs-translations/ko-KR)
- [日语](https://github.com/electron/electron/tree/master/docs-translations/jp)
- [西班牙语](https://github.com/electron/electron/tree/master/docs-translations/es)
- [简体中文](https://github.com/electron/electron/tree/master/docs-translations/zh-CN)
- [繁体中文](https://github.com/electron/electron/tree/master/docs-translations/zh-TW)
- [土耳其](https://github.com/electron/electron/tree/master/docs-translations/tr-TR)
- [乌克兰](https://github.com/electron/electron/tree/master/docs-translations/uk-UA)
- [俄语](https://github.com/electron/electron/tree/master/docs-translations/ru-RU)
- [法语](https://github.com/electron/electron/tree/master/docs-translations/fr-FR)

## 快速开始

Clone 并 run 这个 [`electron/electron-quick-start`](https://github.com/electron/electron-quick-start)
库来启动一个最小项目

## 社区

你可以在此提问题或互相提供帮助:
- [`electron`](http://discuss.atom.io/c/electron) 上的Atom论坛
- `#atom-shell` Freenode上的聊天频道
- [`Atom`](http://atom-slack.herokuapp.com/) Slack上的频道
- [`electron-br`](https://electron-br.slack.com) *(葡萄牙语－巴西)*
- [`electron-kr`](http://www.meetup.com/electron-kr/) *(韩国)*
- [`electron-jp`](https://electron-jp-slackin.herokuapp.com/) *(日语)*
- [`electron-tr`](http://www.meetup.com/Electron-JS-Istanbul/) *(土耳其)*
- [`electron-id`](https://electron-id.slack.com) *(印度尼西亚)*

查看 [awesome-electron](https://github.com/sindresorhus/awesome-electron)
例子，工具和资源列表。
