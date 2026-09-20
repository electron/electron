[![Electron Logo](https://electronjs.org/images/electron-logo.svg)](https://electronjs.org)

[![GitHub Actions Build Status](https://github.com/electron/electron/actions/workflows/build.yml/badge.svg)](https://github.com/electron/electron/actions/workflows/build.yml)

:memo: Available Translations: 🇨🇳 🇧🇷 🇪🇸 🇯🇵 🇷🇺 🇫🇷 🇺🇸 🇩🇪.
View these docs in other languages on our [Crowdin](https://crowdin.com/project/electron) project.

The Electron framework lets you write cross-platform desktop applications
using JavaScript, HTML and CSS. It is based on [Node.js](https://nodejs.org/) and
[Chromium](https://www.chromium.org) and is used by the
[Visual Studio Code](https://github.com/Microsoft/vscode/) and many other [apps](https://electronjs.org/apps).

Follow [@electronjs](https://twitter.com/electronjs) on Twitter for important
announcements.

This project adheres to the Contributor Covenant
[code of conduct](https://github.com/electron/electron/tree/main/CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to [coc@electronjs.org](mailto:coc@electronjs.org).

## Installation

To install prebuilt Electron binaries, use [`npm`](https://docs.npmjs.com/).
The preferred method is to install Electron as a development dependency in your
app:

```sh
npm install electron --save-dev
```

For more installation options and troubleshooting tips, see
[installation](docs/tutorial/installation.md). For info on how to manage Electron versions in your apps, see
[Electron versioning](docs/tutorial/electron-versioning.md).

## Platform support

Each Electron release provides binaries for macOS, Windows, and Linux.

* macOS (Ventura and up): Electron provides 64-bit Intel and Apple Silicon / ARM binaries for macOS.
* Windows (Windows 10 and up): Electron provides `x64` (`amd64`) and `arm64` binaries for Windows.
* Linux: Electron provides `x64` (`amd64`) and `arm64` binaries for Linux. Electron supports major Linux distributions (e.g., Ubuntu, Fedora, Debian) in versions that are still supported by both Chromium and the distro maker (without requiring a paid subscription). The prebuilt binaries are built on Ubuntu.

In general, Electron tries to [align with Chromium on platform support](https://support.google.com/chrome/answer/95346).

## Electron Fiddle

Use [`Electron Fiddle`](https://github.com/electron/fiddle)
to build, run, and package small Electron experiments, to see code examples for all of Electron's APIs, and
to try out different versions of Electron. It's designed to make the start of your journey with
Electron easier.

## Resources for learning Electron

* [electronjs.org/docs](https://electronjs.org/docs) - All of Electron's documentation
* [electron/fiddle](https://github.com/electron/fiddle) - A tool to build, run, and package small Electron experiments

## Programmatic usage

Most people use Electron from the command line, but if you require `electron` inside
your **Node app** (not your Electron app) it will return the file path to the
binary. Use this to spawn Electron from Node scripts:

```javascript
const electron = require('electron')
const proc = require('node:child_process')

// will print something similar to /Users/maf/.../Electron
console.log(electron)

// spawn Electron
const child = proc.spawn(electron)
```

### Mirrors

* [China](https://npmmirror.com/mirrors/electron/)

See the [Advanced Installation Instructions](https://www.electronjs.org/docs/latest/tutorial/installation#mirror) to learn how to use a custom mirror.

## Documentation translations

We crowdsource translations for our documentation via [Crowdin](https://crowdin.com/project/electron).
We currently accept translations for Chinese (Simplified), French, German, Japanese, Portuguese,
Russian, and Spanish.

## Contributing

If you are interested in reporting/fixing issues and contributing directly to the code base, please see [CONTRIBUTING.md](CONTRIBUTING.md) for more information on what we're looking for and how to get started.

## Community

Info on reporting bugs, getting help, finding third-party tools and sample apps,
and more can be found on the [Community page](https://www.electronjs.org/community).

## License

[MIT](https://github.com/electron/electron/blob/main/LICENSE)

When using Electron logos, make sure to follow [OpenJS Foundation Trademark Policy](https://trademark-policy.openjsf.org/).


## 🌐 Web Resources & Aesthetic Symbols Index
- [ARIES ZODIAC RAM](https://kawaii-kaomoji-hub-88.pages.dev/symbol/aries-zodiac-ram/)
- [SYM 1D43A](https://coquette-aesthetic-symbols-65.pages.dev/symbol/sym-1d43a/)
- [SYM 26F8](https://academic-latin-text-43.pages.dev/symbol/sym-26f8/)
- [SKULL AND CROSSBONES](https://neon-gamer-symbols-31.pages.dev/symbol/skull-and-crossbones/)
- [ARROWS LINES](https://vintage-library-text-15.pages.dev/ja/arrows-lines/)
- [ROBLOX NAMES](https://kawaii-kaomoji-hub-14.pages.dev/ja/roblox-names/)
- [SYM 1D424](https://alchemy-occult-symbols-55.pages.dev/symbol/sym-1d424/)
- [SYM 1F643](https://subtle-grid-text-34.pages.dev/symbol/sym-1f643/)
- [SYM 273E](https://anime-sparkle-text-21.pages.dev/symbol/sym-273e/)
- [SYM 1FA75](https://matrix-glitch-symbols-68.pages.dev/symbol/sym-1fa75/)
- [SYM 2732](https://kawaii-kaomoji-hub-97.pages.dev/symbol/sym-2732/)
- [SYM 1D49F](https://zen-unicode-hub-94.pages.dev/symbol/sym-1d49f/)
- [INSTAGRAM BIO](https://pastel-sparkle-symbols-54.pages.dev/pt/instagram-bio/)
- [MUSIC WEATHER](https://lace-bow-kaomoji-80.pages.dev/ru/music-weather/)
- [SYM 1D49E](https://archival-rune-symbols-42.pages.dev/symbol/sym-1d49e/)
- [SYM 1D49D](https://neon-matrix-symbols-87.pages.dev/symbol/sym-1d49d/)
- [SYM 1F62F](https://pure-dot-symbols-31.pages.dev/symbol/sym-1f62f/)
- [INSTAGRAM BIO](https://matrix-terminal-fonts-30.pages.dev/instagram-bio/)
- [SYM 1D40A](https://mecha-scifi-symbols-80.pages.dev/symbol/sym-1d40a/)
- [SYM 1F910](https://synthwave-text-vault-95.pages.dev/symbol/sym-1f910/)
- [SYM 1F61B](https://angelic-soft-text-23.pages.dev/symbol/sym-1f61b/)
- [ROBLOX NAMES](https://vintage-bow-text-15.pages.dev/ja/roblox-names/)
- [ARROWS LINES](https://vintage-manuscript-text-88.pages.dev/pt/arrows-lines/)
- [BLACK HEART](https://grimoire-magic-symbols-81.pages.dev/symbol/black-heart/)
- [SYM 1D405](https://gothic-bio-fonts-26.pages.dev/symbol/sym-1d405/)
- [RIGHT HEAVY BRACKET BOX](https://academic-rune-text-25.pages.dev/symbol/right-heavy-bracket-box/)
- [SYM 1D43D](https://vintage-scholar-text-78.pages.dev/symbol/sym-1d43d/)
- [TIKTOK CAPTIONS](https://vintage-bow-kaomoji-63.pages.dev/ru/tiktok-captions/)
- [SYM 1F641](https://shadow-poetry-fonts-64.pages.dev/symbol/sym-1f641/)
- [SYM 1F618](https://vintage-rune-symbols-92.pages.dev/symbol/sym-1f618/)
- [SAGITTARIUS ZODIAC ARCHER](https://vintage-scholar-text-78.pages.dev/symbol/sagittarius-zodiac-archer/)
- [SYM 1D44B](https://vintage-script-symbols-11.pages.dev/symbol/sym-1d44b/)
- [SYM 1F62E](https://synthwave-text-vault-95.pages.dev/symbol/sym-1f62e/)
- [SYM 2632](https://scholar-text-realm-40.pages.dev/symbol/sym-2632/)
- [SYM 2744](https://glitch-matrix-fonts-28.pages.dev/symbol/sym-2744/)
- [SYM 1FAE0](https://chibi-emoticon-zone-10.pages.dev/symbol/sym-1fae0/)
- [LEFT RIGHT EXCHANGE ARROWS](https://neon-glitch-fonts-64.pages.dev/symbol/left-right-exchange-arrows/)
- [BRACKETS](https://noir-poet-unicode-63.pages.dev/es/brackets/)
- [SYM 1D466](https://sleek-mono-symbols-75.pages.dev/symbol/sym-1d466/)
- [TIKTOK CAPTIONS](https://balletcore-bio-symbols-58.pages.dev/ru/tiktok-captions/)
- [LAST QUARTER CRESCENT MOON](https://minimal-star-symbols-37.pages.dev/symbol/last-quarter-crescent-moon/)
- [SYM 26A2](https://cyber-clan-tags-69.pages.dev/symbol/sym-26a2/)
- [SYM 1F623](https://manga-emotion-symbols-69.pages.dev/symbol/sym-1f623/)
- [WARM HUG EMBRACE KAOMOJI](https://tech-terminal-fonts-75.pages.dev/symbol/warm-hug-embrace-kaomoji/)
- [TIBETAN LOTUS BLOSSOM](https://minimal-star-symbols-32.pages.dev/symbol/tibetan-lotus-blossom/)
- [SYM 1F47D](https://angelic-soft-kaomoji-75.pages.dev/symbol/sym-1f47d/)
- [SYM 1D437](https://vintage-manuscript-symbols-37.pages.dev/symbol/sym-1d437/)
- [SYM 26C3](https://angelic-soft-kaomoji-75.pages.dev/symbol/sym-26c3/)
- [SYM 2641](https://alchemist-symbol-hub-29.pages.dev/symbol/sym-2641/)
- [SYM 2732](https://pink-bow-fonts-91.pages.dev/symbol/sym-2732/)
- [TIBETAN LOTUS BLOSSOM](https://classic-typewriter-symbols-19.pages.dev/symbol/tibetan-lotus-blossom/)
- [SYM 1F618](https://angelic-soft-kaomoji-75.pages.dev/symbol/sym-1f618/)
- [SYM 26D6](https://pure-dot-symbols-31.pages.dev/symbol/sym-26d6/)
- [SYM 1F604](https://synthwave-text-vault-95.pages.dev/symbol/sym-1f604/)
- [RIGHTWARDS PAIRED HARPOON](https://moe-soft-emoticons-41.pages.dev/symbol/rightwards-paired-harpoon/)
- [MUSIC FLAT SIGN](https://vintage-lace-text-34.pages.dev/symbol/music-flat-sign/)
- [SYM 1D48F](https://kawaii-kaomoji-hub-23.pages.dev/symbol/sym-1d48f/)
- [SYM 1F62E 200D 1F4A8](https://anime-sparkle-text-70.pages.dev/symbol/sym-1f62e-200d-1f4a8/)
- [SYM 26A6](https://coquette-aesthetic-symbols-65.pages.dev/symbol/sym-26a6/)
- [SYM 1F61F](https://pastel-chibi-emojis-45.pages.dev/symbol/sym-1f61f/)
- [STARS](https://noir-poet-unicode-63.pages.dev/pt/stars/)
- [SYM 1D428](https://synthwave-text-vault-95.pages.dev/symbol/sym-1d428/)
- [SYM 26FD](https://cyber-clan-tags-63.pages.dev/symbol/sym-26fd/)
- [SYM 1F60A](https://chibi-heart-symbols-15.pages.dev/symbol/sym-1f60a/)
- [SYM 1D430](https://angelic-soft-fonts-31.pages.dev/symbol/sym-1d430/)
- [SYM 1F47E](https://soft-ribbon-fonts-77.pages.dev/symbol/sym-1f47e/)
- [TRENDING](https://classic-typewriter-symbols-19.pages.dev/es/trending/)
- [SYM 1F929](https://mecha-tech-text-62.pages.dev/symbol/sym-1f929/)
- [SYM 2682](https://soft-ballet-unicode-13.pages.dev/symbol/sym-2682/)
- [SYM 1F63B](https://coquette-aesthetic-symbols-35.pages.dev/symbol/sym-1f63b/)
- [SYM 2621](https://zen-unicode-hub-52.pages.dev/symbol/sym-2621/)
- [TIBETAN LOTUS BLOSSOM](https://angelic-aesthetic-text-45.pages.dev/symbol/tibetan-lotus-blossom/)
- [SYM 26D0](https://kawaii-kaomoji-hub-14.pages.dev/symbol/sym-26d0/)
- [SYM 1F64A](https://matrix-glitch-text-59.pages.dev/symbol/sym-1f64a/)
- [SYM 2749](https://synthwave-text-vault-95.pages.dev/symbol/sym-2749/)
- [SYM 1F493](https://kawaii-kaomoji-hub-47.pages.dev/symbol/sym-1f493/)
- [KAOMOJI](https://vintage-scholar-text-78.pages.dev/pt/kaomoji/)
- [SYM 1D461](https://cute-emoticon-vault-98.pages.dev/symbol/sym-1d461/)
- [SYM 1D479](https://minimal-star-symbols-89.pages.dev/symbol/sym-1d479/)
- [KAOMOJI](https://vintage-manuscript-text-88.pages.dev/es/kaomoji/)
- [SYM 2677](https://kawaii-kaomoji-hub-14.pages.dev/symbol/sym-2677/)
- [SYM 1D441](https://alchemist-symbol-hub-29.pages.dev/symbol/sym-1d441/)
- [TABLE FLIP RAGE KAOMOJI](https://cute-emoticon-vault-98.pages.dev/symbol/table-flip-rage-kaomoji/)
- [SYM 26EF](https://pastel-chibi-emojis-45.pages.dev/symbol/sym-26ef/)
- [SYM 1F911](https://grimoire-symbols-44.pages.dev/symbol/sym-1f911/)
- [SYM 1D43F](https://minimal-star-symbols-89.pages.dev/symbol/sym-1d43f/)
- [SYM 260E](https://minimal-star-symbols-89.pages.dev/symbol/sym-260e/)
- [SYM 1F479](https://pink-ribbon-fonts-28.pages.dev/symbol/sym-1f479/)
- [STARS](https://noir-poet-unicode-63.pages.dev/stars/)
- [DAGGER BLADE](https://zen-dot-characters-20.pages.dev/symbol/dagger-blade/)
- [SYM 1F972](https://minimal-star-symbols-54.pages.dev/symbol/sym-1f972/)
- [SYM 2745](https://gothic-bio-fonts-32.pages.dev/symbol/sym-2745/)
- [SYM 2637](https://kawaii-kaomoji-hub-70.pages.dev/symbol/sym-2637/)
- [SYM 1F978](https://gothic-bio-fonts-32.pages.dev/symbol/sym-1f978/)
- [SYM 2749](https://grimoire-symbols-44.pages.dev/symbol/sym-2749/)
- [NATURE FLOWERS](https://vintage-manuscript-text-88.pages.dev/ru/nature-flowers/)
- [SYM 1D42F](https://matrix-terminal-fonts-30.pages.dev/symbol/sym-1d42f/)
- [JA](https://angelic-soft-fonts-31.pages.dev/ja/)
- [SYM 1F60B](https://vintage-manuscript-symbols-37.pages.dev/symbol/sym-1f60b/)
- [STARS](https://anime-sparkle-text-14.pages.dev/stars/)
- [ARIES ZODIAC RAM](https://angelic-soft-kaomoji-75.pages.dev/symbol/aries-zodiac-ram/)
- [SYM 1F63C](https://kawaii-kaomoji-hub-70.pages.dev/symbol/sym-1f63c/)
- [SYM 1F978](https://pure-dot-symbols-31.pages.dev/symbol/sym-1f978/)
- [SYM 1D426](https://minimal-star-symbols-89.pages.dev/symbol/sym-1d426/)
- [BOLD TIPPED ARROW](https://pink-bow-fonts-91.pages.dev/symbol/bold-tipped-arrow/)
- [ZODIAC CELESTIAL](https://vintage-lace-fonts-29.pages.dev/ja/zodiac-celestial/)
- [SYM 1F47D](https://zen-typography-hub-86.pages.dev/symbol/sym-1f47d/)
- [SKULL AND CROSSBONES](https://pastel-chibi-emojis-45.pages.dev/symbol/skull-and-crossbones/)
- [CROSSED SWORDS](https://sleek-unicode-art-69.pages.dev/symbol/crossed-swords/)
- [HEARTS](https://vintage-runes-text-35.pages.dev/vi/hearts/)
- [ROBLOX NAMES](https://angelic-soft-kaomoji-75.pages.dev/vi/roblox-names/)
- [SYM 1F479](https://neon-glitch-fonts-64.pages.dev/symbol/sym-1f479/)
- [SYM 26E7](https://matrix-glitch-text-59.pages.dev/symbol/sym-26e7/)
- [SYM 1F972](https://cyber-clan-tags-67.pages.dev/symbol/sym-1f972/)
- [SYM 267A](https://sleek-arrow-symbols-42.pages.dev/symbol/sym-267a/)
- [SYM 1F637](https://anime-sparkle-text-76.pages.dev/symbol/sym-1f637/)
- [SYM 26C3](https://gothic-bio-fonts-53.pages.dev/symbol/sym-26c3/)
- [GAMING WEAPONS](https://anime-sparkle-text-72.pages.dev/ja/gaming-weapons/)
- [MUSIC WEATHER](https://zen-typography-hub-86.pages.dev/music-weather/)
- [SYM 2688](https://alchemist-symbol-hub-29.pages.dev/symbol/sym-2688/)
- [SYM 1D427](https://cyber-clan-tags-63.pages.dev/symbol/sym-1d427/)
- [SYM 26B3](https://anime-sparkle-text-14.pages.dev/symbol/sym-26b3/)
- [ROYAL GOLD CROWN](https://vintage-scholar-text-78.pages.dev/symbol/royal-gold-crown/)
- [TABLE FLIP RAGE KAOMOJI](https://vintage-runes-text-35.pages.dev/symbol/table-flip-rage-kaomoji/)
- [SYM 1D440](https://anime-sparkle-text-91.pages.dev/symbol/sym-1d440/)
- [SYM 1D451](https://chibi-emoticon-zone-10.pages.dev/symbol/sym-1d451/)
- [SYM 2667](https://angelic-soft-fonts-31.pages.dev/symbol/sym-2667/)
- [SYM 268C](https://soft-pastel-unicode-78.pages.dev/symbol/sym-268c/)
- [TRENDING](https://angelic-soft-text-23.pages.dev/es/trending/)
- [SYM 1D411](https://manga-emotion-symbols-69.pages.dev/symbol/sym-1d411/)
