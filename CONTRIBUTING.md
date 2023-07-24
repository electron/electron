# Contributing to Electron

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

This project adheres to the Contributor Covenant [code of conduct](CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to coc@electronjs.org.

The following is a set of guidelines for contributing to Electron.
These are just guidelines, not rules, use your best judgment and feel free to
propose changes to this document in a pull request.

## [Issues](https://electronjs.org/docs/development/issues)

Issues are created [here](https://github.com/electron/electron/issues/new).

* [How to Contribute in Issues](https://electronjs.org/docs/development/issues#how-to-contribute-in-issues)
* [Asking for General Help](https://electronjs.org/docs/development/issues#asking-for-general-help)
* [Submitting a Bug Report](https://electronjs.org/docs/development/issues#submitting-a-bug-report)
* [Triaging a Bug Report](https://electronjs.org/docs/development/issues#triaging-a-bug-report)
* [Resolving a Bug Report](https://electronjs.org/docs/development/issues#resolving-a-bug-report)

### Issue Closure

Bug reports will be closed if the issue has been inactive and the latest affected version no longer receives support. At the moment, Electron maintains its three latest major versions, with a new major version being released every 8 weeks. (For more information on Electron's release cadence, see [this blog post](https://electronjs.org/blog/8-week-cadence).)

_If an issue has been closed and you still feel it's relevant, feel free to ping a maintainer or add a comment!_

### Languages

We accept issues in _any_ language.
When an issue is posted in a language besides English, it is acceptable and encouraged to post an English-translated copy as a reply.
Anyone may post the translated reply.
In most cases, a quick pass through translation software is sufficient.
Having the original text _as well as_ the translation can help mitigate translation errors.

Responses to posted issues may or may not be in the original language.

**Please note** that using non-English as an attempt to circumvent our [Code of Conduct](https://github.com/electron/electron/blob/main/CODE_OF_CONDUCT.md) will be an immediate, and possibly indefinite, ban from the project.

## [Pull Requests](https://electronjs.org/docs/development/pull-requests)

Pull Requests are the way concrete changes are made to the code, documentation,
dependencies, and tools contained in the `electron/electron` repository.

* [Setting up your local environment](https://electronjs.org/docs/development/pull-requests#setting-up-your-local-environment)
  * [Step 1: Fork](https://electronjs.org/docs/development/pull-requests#step-1-fork)
  * [Step 2: Build](https://electronjs.org/docs/development/pull-requests#step-2-build)
  * [Step 3: Branch](https://electronjs.org/docs/development/pull-requests#step-3-branch)
* [Making Changes](https://electronjs.org/docs/development/pull-requests#making-changes)
  * [Step 4: Code](https://electronjs.org/docs/development/pull-requests#step-4-code)
  * [Step 5: Commit](https://electronjs.org/docs/development/pull-requests#step-5-commit)
    * [Commit message guidelines](https://electronjs.org/docs/development/pull-requests#commit-message-guidelines)
  * [Step 6: Rebase](https://electronjs.org/docs/development/pull-requests#step-6-rebase)
  * [Step 7: Test](https://electronjs.org/docs/development/pull-requests#step-7-test)
  * [Step 8: Push](https://electronjs.org/docs/development/pull-requests#step-8-push)
  * [Step 9: Opening the Pull Request](https://electronjs.org/docs/development/pull-requests#step-9-opening-the-pull-request)
  * [Step 10: Discuss and Update](https://electronjs.org/docs/development/pull-requests#step-10-discuss-and-update)
    * [Approval and Request Changes Workflow](https://electronjs.org/docs/development/pull-requests#approval-and-request-changes-workflow)
  * [Step 11: Landing](https://electronjs.org/docs/development/pull-requests#step-11-landing)
  * [Continuous Integration Testing](https://electronjs.org/docs/development/pull-requests#continuous-integration-testing)

### Dependencies Upgrades Policy

Dependencies in Electron's `package.json` or `yarn.lock` files should only be altered by maintainers. For security reasons, we will not accept PRs that alter our `package.json` or `yarn.lock` files. We invite contributors to make requests updating these files in our issue tracker. If the change is significantly complicated, draft PRs are welcome, with the understanding that these PRs will be closed in favor of a duplicate PR submitted by an Electron maintainer.

## Style Guides

See [Coding Style](https://electronjs.org/docs/development/coding-style) for information about which standards Electron adheres to in different parts of its codebase.

## Further Reading

For more in-depth guides on developing Electron, see
[/docs/development](/docs/development/README.md)
