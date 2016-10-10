# C++ 코드에서 clang-format 사용하기

[`clang-format`](http://clang.llvm.org/docs/ClangFormat.html) 은
C/C++/Objective-C 코드 자동 서식 맞춤 도구입니다. 그래서 개발자들은 코드 검토시에
스타일 문제에 대해 걱정할 필요가 없습니다.

풀 리퀘스트를 하기 전에 변경된 C++ 코드의 서식을 맞추는 것이 좋습니다. 이것은
당신과 검토자들의 시간을 절약해 줄 것입니다.

`npm install -g clang-format` 으로 `clang-format` 과 `git-clang-format` 을
설치할 수 있습니다.

Electron C++ 코드 형식에 따라 자동 서식 맞춤을 하려면, 간단하게
`clang-format -i path/to/electron/file.cc` 를 실행하면 됩니다. 이것은
macOS/Linux/Windows 에서 작동합니다.

변경된 코드를 서식에 맞추는 작업 흐름:

1. Electron 저장소의 코드를 변경합니다.
2. `git add your_changed_file.cc` 를 실행합니다.
3. `git-clang-format` 를 실행하면, `your_changed_file.cc` 에서 변경을 확인할 수
  있습니다. 이 변경은 `clang-format` 에서 생성됩니다.
4. `git add your_changed_file.cc` 를 실행하고, 변경사항을 커밋합니다.
5. 이제 브랜치를 풀 리퀘스트 할 수 있습니다.

마지막 git 커밋 (HEAD) 의 변경된 내용의 서식을 맞추려면,
`git-clang-format HEAD~1` 을 실행하면 됩니다. 자세한 내용은
`git-clang-format -h` 를 보세요.

## 편집기 통합

`clang-format` 을 당신이 선호하는 편집기에 통합할 수 있습니다.
편집기 통합을 설정하는 방법에 대한 자세한 지침은 다음 페이지를 참조하세요:

  * [Atom](https://atom.io/packages/clang-format)
  * [Vim & Emacs](http://clang.llvm.org/docs/ClangFormat.html#vim-integration)
