[![Electron Logo](https://electronjs.org/images/electron-logo.svg)](https://electronjs.org)

[![GitHub Actions Build Status](https://github.com/electron/electron/actions/workflows/build.yml/badge.svg)](https://github.com/electron/electron/actions/workflows/build.yml)
[![Electron Discord Invite](https://img.shields.io/discord/745037351163527189?color=%237289DA&label=chat&logo=discord&logoColor=white)](https://discord.gg/electronjs)

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
* [electronjs.org/community#boilerplates](https://electronjs.org/community#boilerplates) - Sample starter apps created by the community

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


## 🌐 Web Resources & Interactive Index
- [MOVE EMOJI](https://studyquesthub.web.app/move-emoji.html)
- [CATEGORY CASUAL969](https://iskillcrafts.pages.dev/category-casual969.html)
- [CAR SIMULATOR 3D CAR GAME 3D](https://studyplaying.github.io/car-simulator-3d-car-game-3d.html)
- [SPRUNKI QUIZ](https://thelearnquester.web.app/sprunki-quiz.html)
- [WORD GAME 2026](https://learnquester.github.io/word-game-2026.html)
- [MOJO MATCH 3D](https://studyquests.pages.dev/mojo-match-3d.html)
- [CATEGORY FLASH](https://thelearnquester.web.app/category-flash.html)
- [CATEGORY 204828](https://thelearnquester.web.app/category-204828.html)
- [MATH RUNNER](https://theskillquest.pages.dev/math-runner.html)
- [SWEET MERGE](https://studyplaying.github.io/sweet-merge.html)
- [CATEGORY TOWER DEFENSE 3](https://theskillquest.pages.dev/category-tower-defense-3.html)
- [QUIZ X](https://studyquests.pages.dev/quiz-x.html)
- [DROP BRICKS BREAKER](https://thelearnquesters.pages.dev/drop-bricks-breaker.html)
- [DARLING DOLL](https://learnquesters.pages.dev/darling-doll.html)
- [CATEGORY SIMULATION 3](https://theskillquest.pages.dev/category-simulation-3.html)
- [CATEGORY SPACE57](https://theskillquest.pages.dev/category-space57.html)
- [HAIR SALON BEAUTY SALON](https://studyquests.pages.dev/hair-salon-beauty-salon.html)
- [CATCH THE PIG](https://studyplaying.github.io/catch-the-pig.html)
- [SCREW MASTERS 3D PUZZLE](https://studyquests.pages.dev/screw-masters-3d-puzzle.html)
- [HEXA SORT TRICK OR TREAT](https://studyquesthub.web.app/hexa-sort-trick-or-treat.html)
- [CATEGORY FPS](https://studyplaying.github.io/category-fps.html)
- [CATEGORY STICKMAN 2](https://theskillquest.pages.dev/category-stickman-2.html)
- [INSPECTOR CAT](https://studyquests.pages.dev/inspector-cat.html)
- [FREDDYS NIGHTMARES RETURN HORROR NEW YEAR](https://studyquests.pages.dev/freddys-nightmares-return-horror-new-year.html)
- [CATEGORY SOLITAIRE27](https://theskillquest.pages.dev/category-solitaire27.html)
- [CATEGORY CONTROLLER 2](https://thelearnquester.web.app/category-controller-2.html)
- [SUPER DOG HERO DASH](https://thelearnquesters.pages.dev/super-dog-hero-dash.html)
- [CATEGORY SURVIVAL366](https://theskillquest.pages.dev/category-survival366.html)
- [INDEX40](https://theskillquest.pages.dev/index40.html)
- [SPIDER EVOLUTION](https://theskillquest.pages.dev/spider-evolution.html)
- [CATEGORY MANAGEMENT209](https://studyquests.pages.dev/category-management209.html)
- [BUBBLE SHOOTER WONDERS OF EGYPT](https://theskillquest.pages.dev/bubble-shooter-wonders-of-egypt.html)
- [DREAM MANIA HAPPY MATCH](https://theskillquest.pages.dev/dream-mania-happy-match.html)
- [CATEGORY CARTOON76](https://studyplaying.github.io/category-cartoon76.html)
- [CATEGORY CONTROLLER 2](https://studyplaying.github.io/category-controller-2.html)
- [TAP HOLD](https://themindzone.pages.dev/tap-hold.html)
- [SKIBIDI SURVIVOR RUSH](https://learnquesters.pages.dev/skibidi-survivor-rush.html)
- [INDEX30](https://studyplaying.github.io/index30.html)
- [STREET RACING MOTO DRIFT](https://studyquests.pages.dev/street-racing-moto-drift.html)
- [MAGIC SORT](https://themindzone.pages.dev/magic-sort.html)
- [MERGEDUELIO](https://iskillquest.pages.dev/mergeduelio.html)
- [2 CARS RUN](https://studyplaying.github.io/2-cars-run.html)
- [FRAGEN](https://studyplaying.github.io/fragen.html)
- [CATEGORY FREE FASHION GAMES](https://themindzone.pages.dev/category-free-fashion-games.html)
- [RED STICKMAN VS MONSTER SCHOOL](https://studyplaying.github.io/red-stickman-vs-monster-school.html)
- [1010 ELIXIR ALCHEMY](https://themindzone.pages.dev/1010-elixir-alchemy.html)
- [PATTERNS](https://studyquests.pages.dev/patterns.html)
- [CATEGORY BIKE](https://studyplaying.github.io/category-bike.html)
- [CATEGORY PROXY LIST](https://iskillquest.pages.dev/category-proxy-list.html)
- [CATEGORY TOP DOWN248](https://themindzone.pages.dev/category-top-down248.html)
- [COLORSFORMS](https://studyplaying.github.io/colorsforms.html)
- [CLONEUP STACK YOURSELF](https://studyplaying.github.io/cloneup-stack-yourself.html)
- [STICKMAN GUYS DEFENSE](https://studyplaying.github.io/stickman-guys-defense.html)
- [SQUID GAME MEMORY CARD MATCH](https://learnquesters.pages.dev/squid-game-memory-card-match.html)
- [CATEGORY CARE](https://thelearnquester.web.app/category-care.html)
- [FASHION WEEK 2025](https://studyquests.pages.dev/fashion-week-2025.html)
- [REAL CAR PARKING SIMULATOR](https://iskillquest.pages.dev/real-car-parking-simulator.html)
- [LOVIE CHICS COACHELLA FESTIVAL](https://studyquesthub.web.app/lovie-chics-coachella-festival.html)
- [HOLE EAT GROW ATTACK](https://studyplaying.github.io/hole-eat-grow-attack.html)
- [BLOCKPUZZLE COLOR BLAST](https://studyplaying.github.io/blockpuzzle-color-blast.html)
- [CATEGORY PENALTY](https://theskillquest.pages.dev/category-penalty.html)
- [CRICKET CLASH PONG](https://studyplaying.github.io/cricket-clash-pong.html)
- [MAKE TWO](https://studyquests.pages.dev/make-two.html)
- [ANIMAL RACING IDLE PARK](https://studyplaying.github.io/animal-racing-idle-park.html)
- [FIGHT TRIVIA](https://studyquests.pages.dev/fight-trivia.html)
- [FROM NERDS TO BEAUTIES](https://studyplaying.github.io/from-nerds-to-beauties.html)
- [POP PUZZLE](https://studyquests.pages.dev/pop-puzzle.html)
- [WORDS FROM WORDS](https://thelearnquesters.pages.dev/words-from-words.html)
- [LABUBU DOLL MUKBANG ASMR UNBLOCKED](https://studyplaying.github.io/labubu-doll-mukbang-asmr-unblocked.html)
- [BLOCK MANIA](https://thelearnquester.web.app/block-mania.html)
- [INDEX31](https://iskillquest.pages.dev/index31.html)
- [INDEX7](https://theskillquest.pages.dev/index7.html)
- [SERIOUS HEAD](https://studyplaying.github.io/serious-head.html)
- [FRUIT MERGE JUICY DROP GAME](https://studyquests.pages.dev/fruit-merge-juicy-drop-game.html)
- [CATEGORY DRESS UP97](https://thelearnquester.web.app/category-dress-up97.html)
- [CATEGORY COOKING](https://studyquests.pages.dev/category-cooking.html)
- [JET FIGHTER AIRPLANE RACING](https://studyplaying.github.io/jet-fighter-airplane-racing.html)
- [HALLOWEEN FRUIT SLICE](https://studyplaying.github.io/halloween-fruit-slice.html)
- [PUSHOVER 3D](https://learnquesters.pages.dev/pushover-3d.html)
- [CATEGORY CUTE](https://iskillquest.pages.dev/category-cute.html)
- [DUCK HUNTING OPEN SEASON](https://themindzone.pages.dev/duck-hunting-open-season.html)
- [CATEGORY CASUAL 8](https://iskillquest.pages.dev/category-casual-8.html)
- [NITRO SPEED CAR RACING](https://studyplaying.github.io/nitro-speed-car-racing.html)
- [TANKS](https://studyplaying.github.io/tanks.html)
- [SID GINNY Y2K GLAM CLASH](https://themindzone.pages.dev/sid-ginny-y2k-glam-clash.html)
- [MR DISC SLINGSHOT STRIKE](https://thelearnquester.web.app/mr-disc-slingshot-strike.html)
- [CUTE ANIMAL WORLD](https://studyquesthub.web.app/cute-animal-world.html)
- [INDEX7](https://thequizzone.pages.dev/index7.html)
- [CATEGORY BALL173](https://studyplaying.github.io/category-ball173.html)
- [LAZY WORKERS](https://studyplaying.github.io/lazy-workers.html)
- [CATEGORY THIRD PERSON SHOOTER80](https://iskillquest.pages.dev/category-third-person-shooter80.html)
- [LABUBA MERGE](https://studyquests.pages.dev/labuba-merge.html)
- [SECRETS OF CHARMLAND](https://studyplaying.github.io/secrets-of-charmland.html)
- [DREAM ROOM MAKEOVER](https://thequizzone.pages.dev/dream-room-makeover.html)
- [STICKMAN DOORS AND ISLAND](https://studyquests.pages.dev/stickman-doors-and-island.html)
- [CATEGORY BOOKMARKLET](https://studyplaying.github.io/category-bookmarklet.html)
- [CATEGORY TOP DOWN251](https://learnquesters.pages.dev/category-top-down251.html)
- [PLANTS VS ZOMBIES WAR](https://studyplaying.github.io/plants-vs-zombies-war.html)
- [CATEGORY CASUAL971](https://iskillquest.pages.dev/category-casual971.html)
- [HALLOWEEN CHALLENGE](https://studyquests.pages.dev/halloween-challenge.html)
- [SNOW RUSH 3D](https://studyplaying.github.io/snow-rush-3d.html)
- [SAVE THE CROP](https://studyplaying.github.io/save-the-crop.html)
- [SLICE IT UP](https://learnquester.pages.dev/slice-it-up.html)
- [PRSINO](https://studyplaying.github.io/prsino.html)
- [CATEGORY CASUAL 9](https://studyplaying.github.io/category-casual-9.html)
- [CATEGORY FARMING87](https://studyquests.pages.dev/category-farming87.html)
- [RESTAURANT VIP MASTERCHEF](https://studyplaying.github.io/restaurant-vip-masterchef.html)
- [INDEX20](https://iskillquest.pages.dev/index20.html)
- [CATEGORY DESTROY](https://studyplaying.github.io/category-destroy.html)
- [MERGE GALAXY](https://studyquests.pages.dev/merge-galaxy.html)
- [CATEGORY CARDS](https://thelearnquester.web.app/category-cards.html)
- [LAMBO TRAFFIC RACER](https://studyquests.github.io/lambo-traffic-racer.html)
- [MEGA RAMP CAR](https://learnquester.pages.dev/mega-ramp-car.html)
- [THE EARTH EVOLUTION](https://learnquesters.pages.dev/the-earth-evolution.html)
- [SORT GAME TOY SORT](https://themindzone.pages.dev/sort-game-toy-sort.html)
- [ANIMAL RACING IDLE PARK](https://studyquests.pages.dev/animal-racing-idle-park.html)
- [DRAWING SQUARES](https://studyquesthub.web.app/drawing-squares.html)
- [LOLLIPOP STACK RUN](https://studyplaying.github.io/lollipop-stack-run.html)
- [DOGS VS ALIENS](https://studyquests.pages.dev/dogs-vs-aliens.html)
- [MY FIRE STATION WORLD](https://studyplaying.github.io/my-fire-station-world.html)
- [SUPER ONION BOY 2](https://learnquester.pages.dev/super-onion-boy-2.html)
- [GIRLS FUN NAIL SALON](https://quizverses.github.io/girls-fun-nail-salon.html)
- [BLOCK EATING SIMULATOR](https://thequizzone.pages.dev/block-eating-simulator.html)
- [BATTLE OF TANK STEEL](https://thelearnquesters.pages.dev/battle-of-tank-steel.html)
- [HOME RUSH THE FISH WAR](https://thelearnquester.web.app/home-rush-the-fish-war.html)
- [CATEGORY SOCCER 2](https://learnquesters.pages.dev/category-soccer-2.html)
- [CATEGORY DRIFTING116](https://quizverses-9d2f2.web.app/category-drifting116.html)
- [UPHILL RUSH 13](https://thelearnquesters.pages.dev/uphill-rush-13.html)
- [CATEGORY BUBBLE SHOOTER](https://studyplaying.github.io/category-bubble-shooter.html)
- [CANDY SMASH](https://themindzone.pages.dev/candy-smash.html)
