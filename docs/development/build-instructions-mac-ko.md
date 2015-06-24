# 빌드 설명서 (Mac)

## 빌드전 요구 사항

* OS X >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external).

If you are using the python downloaded by Homebrew, you also need to install
following python modules:

* pyobjc

## 코드 가져오기

```bash
$ git clone https://github.com/atom/electron.git
```

## 부트 스트랩

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build Electron so
there is no Xcode project generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## 빌드 하기

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c D
```

After building is done, you can find `Electron.app` under `out/D`.

## 32비트 지원

Electron can only be built for 64bit target on OS X, and there is no plan to
support 32bit OS X in future.

## 테스트

```bash
$ ./script/test.py
```
