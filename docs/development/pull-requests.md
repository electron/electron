# Pull Requests

* [Setting up your local environment](#setting-up-your-local-environment)
  * [Step 1: Fork](#step-1-fork)
  * [Step 2: Build](#step-2-build)
  * [Step 3: Branch](#step-3-branch)
* [The Process of Making Changes](#the-process-of-making-changes)
  * [Step 4: Code](#step-4-code)
  * [Step 5: Commit](#step-5-commit)
    * [Commit message guidelines](#commit-message-guidelines)
  * [Step 6: Rebase](#step-6-rebase)
  * [Step 7: Test](#step-7-test)
  * [Step 8: Push](#step-8-push)
  * [Step 8: Opening the Pull Request](#step-8-opening-the-pull-request)
  * [Step 9: Discuss and Update](#step-9-discuss-and-update)
    * [Approval and Request Changes Workflow](#approval-and-request-changes-workflow)
  * [Step 10: Landing](#step-10-landing)
  * [Continuous Integration Testing](#continuous-integration-testing)

## Setting up your local environment

### Step 1: Fork

Fork the project [on GitHub](https://github.com/electron/electron) and clone your fork
locally.

```text
$ git clone git@github.com:username/electron.git
$ cd electron
$ git remote add upstream https://github.com/electron/electron.git
$ git fetch upstream
```

### Step 2: Build

Build steps and dependencies differ slightly depending on your operating system;
for detailed guides on what is needed to build Electron locally see:
* [Building on MacOS](https://electronjs.org/docs/development/build-instructions-osx)
* [Building on Linux](https://electronjs.org/docs/development/build-instructions-linux)
* [Building on Windows](https://electronjs.org/docs/development/build-instructions-windows)

Once you've built the project locally, you're ready to get started making some changes!

### Step 3: Branch

As a best practice to keep your development environment as organized as
possible, create local branches to work within. These should also be created
directly off of the `master` branch.

```text
$ git checkout -b my-branch -t upstream/master
```

## The Process of Making Changes

### Step 4: Code

The vast majority of pull requests opened against the `electron/electron`
repository includes changes to either the C/C++ code contained in the `atom` or
`brightray` directories, the JavaScript code contained in the `lib` directory, the
documentation in `docs/api` or tests within the `spec` directory.

If you are modifying code, please be sure to run `npm run lint` from time to
time to ensure that the changes follow the code style enforced by the Electron
project.

See [coding style](https://electronjs.org/docs/development/coding-style) for
more information about best practice when modifying code in different parts of
the project.

### Step 5: Commit

It is a recommended best practice to keep your changes as logically grouped
as possible within individual commits. There is no limit to the number of
commits any single pull request may have, and many contributors find it easier
to review changes that are split across multiple commits.

```text
$ git add my/changed/files
$ git commit
```

Note that multiple commits often get squashed when they are landed.

#### Commit message guidelines

A good commit message should describe what changed and why.

1. The first line should:
   - contain a short description of the change (preferably 50 characters or less,
     and no more than 72 characters)
   - be entirely in lowercase with the exception of proper nouns, acronyms, and
   the words that refer to code, like function/variable names

   Examples:
   - `updated osx build documentation for new sdk`
   - `fixed typos in atom_api_menu.h`


2. Keep the second line blank.
3. Wrap all other lines at 72 columns.

### Step 6: Rebase

As a best practice, once you have committed your changes, it is a good idea
to use `git rebase` (not `git merge`) to synchronize your work with the main
repository.

```text
$ git fetch upstream
$ git rebase upstream/master
```

This ensures that your working branch has the latest changes from `electron/electron`
master.

### Step 7: Test

Bug fixes and features should always come with tests. A
[testing guide](https://electronjs.org/docs/development/testing) has been
provided to make the process easier. Looking at other tests to see how they
should be structured can also help.

Before submitting your changes in a pull request, always run the full
test suite. To run the tests:

```text
$ npm run test
```

Make sure the linter does not report any issues and that all tests pass. Please
do not submit patches that fail either check.

If you are updating tests and just want to run a single spec to check it:

```text
$ npm run test -match=menu
```

The above would only run spec modules matching `menu`, which is useful for
anyone who's working on tests that would otherwise be at the very end of
the testing cycle.

### Step 8: Push

Once you are sure your commits are ready to go, with passing tests and linting,
begin the process of opening a pull request by pushing your working branch to
your fork on GitHub.

```text
$ git push origin my-branch
```

### Step 8: Opening the Pull Request

From within GitHub, opening a new pull request will present you with a template
that should be filled out:

```markdown
<!--
Thank you for your Pull Request. Please provide a description above and review
the requirements below.

Bug fixes and new features should include tests and possibly benchmarks.

Contributors guide: https://github.com/electron/electron/blob/master/CONTRIBUTING.md
-->
```

### Step 9: Discuss and update

You will probably get feedback or requests for changes to your pull request.
This is a big part of the submission process so don't be discouraged! Some
contributors may sign off on the pull request right away, others may have
more detailed comments or feedback. This is a necessary part of the process
in order to evaluate whether the changes are correct and necessary.

To make changes to an existing pull request, make the changes to your local
branch, add a new commit with those changes, and push those to your fork.
GitHub will automatically update the pull request.

```text
$ git add my/changed/files
$ git commit
$ git push origin my-branch
```

There are a number of more advanced mechanisms for managing commits using
`git rebase` that can be used, but are beyond the scope of this guide.

Feel free to post a comment in the pull request to ping reviewers if you are
awaiting an answer on something. If you encounter words or acronyms that
seem unfamiliar, refer to this
[glossary](https://sites.google.com/a/chromium.org/dev/glossary).

#### Approval and Request Changes Workflow

All pull requests require approval from a [Code Owner](https://github.com/orgs/electron/teams/code-owners) of the area you
modified in order to land. Whenever a maintainer reviews a pull request they
may find specific details that they would like to see changed or fixed.
These may be as simple as fixing a typo, or may involve substantive changes to
the code you have written. In general, such requests are intended to be helpful, but at times may come across as abrupt or unhelpful, especially requests to change things
that do not include concrete suggestions on *how* to change them.

Try not to be discouraged. If you feel that a particular review is unfair,
say so, or contact one of the other contributors in the project and seek their
input. Often such comments are the result of the reviewer having only taken a
short amount of time to review and are not ill-intended. Such issues can often
be resolved with a bit of patience. That said, reviewers should be expected to
be helpful in their feedback, and feedback that is simply vague, dismissive and
unhelpful is likely safe to ignore.

### Step 10: Landing

In order to land, a pull request needs to be reviewed and approved by
at least one Electron Code Owner and pass CI. After that, as long as there are no
objections from other contributors, the pull request can be merged.

Congratulations and thanks for your contribution!

### Continuous Integration Testing

Every pull request needs to be tested to make sure that it works on the
platforms that Electron supports. This is done by running the code through the
CI system.

Ideally, the code change will pass ("be green") on all platform configurations
supported by Electron. This means that all tests pass and there are no linting
errors. In reality, however, it is not uncommon for the CI infrastructure itself
to fail on specific platforms or for so-called "flaky" tests to fail ("be red").
It is vital to visually inspect the results of all failed ("red") tests to determine
whether the failure was caused by the changes in the pull request.

Only [Releasers](https://github.com/orgs/electron/teams/releasers/members) can
start a CI run. CI will automatically start when you open a pull request, but
if for some reason CI seems to be failing on what you believe is an flake and
you would like it to be rerun, one of them will be happy to do it for you.

