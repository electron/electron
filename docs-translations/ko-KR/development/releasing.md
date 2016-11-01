# 릴리즈

이 문서는 Electron 의 새버전 출시 절차를 설명합니다.

## 릴리즈 노트 편집

현재 절차는 로컬 파일을 유지하고 병합된 풀 요청과 같은 중요한 변화의 추척을
보존하는 것 입니다. 노트 형식에 대한 예제는, [릴리즈 페이지]에서 이전 릴리즈를
보세요.

## 임시 브랜치 생성

`release` 이름의 새 브랜치를 `master` 로부터 생성합니다.

```sh
git checkout master
git pull
git checkout -b release
```

이 브랜치는 임시 릴리즈 브랜치가 생성되고 CI 빌드가 완료되는 동안 아무도 모르는
PR 병합을 방지하기위한 예방조치로써 생성됩니다.

## 버전 올리기

`major`, `minor`, `patch` 를 인수로 전달하여, `bump-version` 스크립트를
실행하세요:

```sh
npm run bump-version -- patch
git push origin HEAD
```

이것은 여러 파일의 버전 번호를 올릴 것 입니다. 예시로 [이 범프 커밋]을 보세요.

대부분의 릴리즈는 `patch` 수준이 될 것입니다. Chrome 또는 다른 주요 변화는
`minor` 를 사용해야합니다. 자세한 정보는, [Electron 버전 관리]를 보세요.

## 릴리즈 초안 편집

1. [릴리즈 페이지]에 가면 릴리즈 노트 초안과 자리 표시자로써 릴리즈 노트를 볼 수
   있습니다.
1. 릴리즈를 편집하고 릴리즈 노트를 추가하세요.
1. 'Save draft' 를 클릭하세요. **'Publish release' 를 누르면 안됩니다!**
1. 모든 빌드가 통과할 때 까지 기다리세요. :hourglass_flowing_sand:

## 임시 브랜치 병합

임시를 마스터로 머지 커밋 생성없이 병합합니다.

```sh
git merge release master --no-commit
git push origin master
```
실패하면, 마스터로 리베이스하고 다시 빌드합니다:

```sh
git pull
git checkout release
git rebase master
git push origin HEAD
```

## 로컬 디버그 빌드 실행

당신이 실제로 원하는 버전을 구축하고 있는지 확인하기 위해 로컬 디버그 빌드를
실행합니다. 때때로 새 버전을 릴리즈하고 있다고 생각하지만, 아닌 경우가 있습니다.

```sh
npm run build
npm start
```

창이 현재 업데이트된 버전을 표시하는지 확인하세요.

## 환경 변수 설정

릴리즈를 게시하려면 다음 환경 변수를 설정해야합니다. 이 자격증명에 대해 다른 팀
구성원에게 문의하세요.

- `ELECTRON_S3_BUCKET`
- `ELECTRON_S3_ACCESS_KEY`
- `ELECTRON_S3_SECRET_KEY`
- `ELECTRON_GITHUB_TOKEN` - "저장소" 영역에 대한 개인 접근 토큰.

이것은 한번만 수행해야합니다.

## 릴리즈 게시

이 스크립트는 바이너리를 내려받고 네이티브 모듈 구축을 위해 node-gyp 으로
윈도우에서 사용되는 노드 헤더와 .lib 링커를 생성합니다.

```sh
npm run release
```

참고: 많은 파이썬의 배포판들은 여전히 오래된 HTTPS 인증서와 함께 제공됩니다.
`InsecureRequestWarning` 를 볼 수 있지만, 무시해도 됩니다.

## 임시 브랜치 삭제

```sh
git checkout master
git branch -D release # 로컬 브랜치 삭제
git push origin :release # 원격 브랜치 삭제
```

[릴리즈 페이지]: https://github.com/electron/electron/releases
[이 범프 커밋]: https://github.com/electron/electron/commit/78ec1b8f89b3886b856377a1756a51617bc33f5a
[Electron 버전 관리]: ../tutorial/electron-versioning.md
