# Issues In Electron

* [How to Contribute to Issues](#how-to-contribute-to-issues)
* [Asking for General Help](#asking-for-general-help)
* [Submitting a Bug Report](#submitting-a-bug-report)
* [Triaging a Bug Report](#triaging-a-bug-report)
* [Resolving a Bug Report](#resolving-a-bug-report)

## How to Contribute to Issues

For any issue, there are fundamentally three ways an individual can
contribute:

1. By opening the issue for discussion: If you believe that you have found
   a new bug in Electron, you should report it by creating a new issue in
   the [`electron/electron` issue tracker](https://github.com/electron/electron/issues).
2. By helping to triage the issue: You can do this either by providing
   assistive details (a reproducible test case that demonstrates a bug) or by
   providing suggestions to address the issue.
3. By helping to resolve the issue: This can be done by demonstrating
   that the issue is not a bug or is fixed; but more often, by opening
   a pull request that changes the source in `electron/electron` in a
   concrete and reviewable manner.

## Asking for General Help

["Finding Support"](../tutorial/support.md#finding-support) has a
list of resources for getting programming help, reporting security issues,
contributing, and more. Please use the issue tracker for bugs only!

## Submitting a Bug Report

To submit a bug report:

When opening a new issue in the [`electron/electron` issue tracker](https://github.com/electron/electron/issues/new/choose), users
will be presented with a template that should be filled in.

```markdown
<!--  As an open source project with a dedicated but small maintainer team, it can sometimes take a long time for issues to be addressed so please be patient and we will get back to you as soon as we can.
-->

### Preflight Checklist
<!-- Please ensure you've completed the following steps by replacing [ ] with [x]-->

* [ ] I have read the [Contributing Guidelines](https://github.com/electron/electron/blob/master/CONTRIBUTING.md) for this project.
* [ ] I agree to follow the [Code of Conduct](https://github.com/electron/electron/blob/master/CODE_OF_CONDUCT.md) that this project adheres to.
* [ ] I have searched the issue tracker for an issue that matches the one I want to file, without success.

### Issue Details

* **Electron Version:**
  * <!-- (output of `node_modules/.bin/electron --version`) e.g. 4.0.3 -->
* **Operating System:**
  * <!-- (Platform and Version) e.g. macOS 10.13.6 / Windows 10 (1803) / Ubuntu 18.04 x64 -->
* **Last Known Working Electron version:**
  * <!-- (if applicable) e.g. 3.1.0 -->

### Expected Behavior
<!-- A clear and concise description of what you expected to happen. -->

### Actual Behavior
<!-- A clear and concise description of what actually happened. -->

### To Reproduce
<!--
Your best chance of getting this bug looked at quickly is to provide an example.
-->

<!--
For bugs that can be encapsulated in a small experiment, you can use Electron Fiddle (https://github.com/electron/fiddle) to publish your example to a GitHub Gist and link it your bug report.
-->

<!--
If Fiddle is insufficient to produce an example, please provide an example REPOSITORY that can be cloned and run. You can fork electron-quick-start (https://github.com/electron/electron-quick-start) and include a link to the branch with your changes.
-->

<!--
If you provide a URL, please list the commands required to clone/setup/run your repo e.g.

$ git clone $YOUR_URL -b $BRANCH
$ npm install
$ npm start || electron .

-->

### Screenshots
<!-- If applicable, add screenshots to help explain your problem. -->

### Additional Information
<!-- Add any other context about the problem here. -->
```

If you believe that you have found a bug in Electron, please fill out this
form to the best of your ability.

The two most important pieces of information needed to evaluate the report are
a description of the bug and a simple test case to recreate it. It is easier to fix
a bug if it can be reproduced.

See [How to create a Minimal, Complete, and Verifiable example](https://stackoverflow.com/help/mcve).

## Triaging a Bug Report

It's common for open issues to involve discussion. Some contributors may
have differing opinions, including whether the behavior is a bug or feature.
This discussion is part of the process and should be kept focused, helpful,
and professional.

Terse responses that provide neither additional context nor supporting detail
are not helpful or professional. To many, such responses are annoying and
unfriendly.

Contributors are encouraged to solve issues collaboratively and help one
another make progress. If you encounter an issue that you feel is invalid, or
which contains incorrect information, explain *why* you feel that way with
additional supporting context, and be willing to be convinced that you may
be wrong. By doing so, we can often reach the correct outcome faster.

## Resolving a Bug Report

Most issues are resolved by opening a pull request. The process for opening and
reviewing a pull request is similar to that of opening and triaging issues, but
carries with it a necessary review and approval workflow that ensures that the
proposed changes meet the minimal quality and functional guidelines of the
Electron project.
