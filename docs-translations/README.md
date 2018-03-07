## Docs Translations

This directory once contained unstructured translations of Electron's 
documentation, but has been deprecated in favor of a new translation process 
using [Crowdin], a GitHub-friendly platform for collaborative translation.

For more details, visit the [electron/i18n] repo.

## Contributing

If you're interested in helping translate Electron's docs, visit 
[Crowdin] and log in with your GitHub account. And thanks!

## Offline Docs

If you miss having access to Electron's raw markdown files in your preferred
language, don't fret! You can still get raw docs, they're just in a 
different place now. See [electron/i18n/tree/master/content]

To more easily view and browse offline docs in your language, clone the repo and use [vmd], 
an Electron-based GitHub-styled markdown viewer:

```sh
npm i -g vmd
git clone https://github.com/electron/i18n
vmd electron-i18n/content/zh-CN
```

[crowdin.com/project/electron]: https://crowdin.com/project/electron
[Crowdin]: https://crowdin.com/project/electron
[electron/i18n]: https://github.com/electron/i18n#readme
[electron/i18n/tree/master/content]: https://github.com/electron/i18n/tree/master/content
[vmd]: http://ghub.io/vmd
