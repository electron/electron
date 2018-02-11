# Issues In Electron

# Issues

* [How to Contribute in Issues](#how-to-contribute-in-issues)
* [Asking for General Help](#asking-for-general-help)
* [Submitting a Bug Report](#submitting-a-bug-report)
* [Triaging a Bug Report](#triaging-a-bug-report)
* [Resolving a Bug Report](#resolving-a-bug-report)

## How to Contribute in Issues

For any issue, there are fundamentally three ways an individual can
contribute:

1. By opening the issue for discussion: For instance, if you believe that you
   have uncovered a bug in Electron, you would report this by creating a new
   issue in the `electron/electron` issue tracker.
2. By helping to triage the issue: You can do this either by providing
   assistive details (a reproducible test case that demonstrates a bug), or by
   providing suggestions for how the issue might be addressed.
3. By helping to resolve the issue: Typically this is done either by
   demonstrating that the issue reported is not actually or is no longer a bug,
   or more often, by opening a pull request that changes as aspect of the source
   in `electron/electron` in a concrete and reviewable manner.

## Asking for General Help

Because the level of activity in the `nodejs/node` repository is so high,
questions or requests for general help using Electron should be directed at
the [community slack channel](https://atomio.slack.com) or the [forum](https://discuss.atom.io/c/electron).

## Submitting a Bug Report

When opening a new issue in the `nodejs/node` issue tracker, users will be
presented with a basic template that should be filled in.

```markdown
<!--
Thanks for opening an issue! A few things to keep in mind:

- The issue tracker is only for bugs and feature requests.
- Before reporting a bug, please try reproducing your issue against
  the latest version of Electron.
- If you need general advice, join our Slack: http://atom-slack.herokuapp.com
-->

* Electron version:
* Operating system:

### Expected behavior

<!-- What do you think should happen? -->

### Actual behavior

<!-- What actually happens? -->

### How to reproduce

<!--

Your best chance of getting this bug looked at quickly is to provide a REPOSITORY that can be cloned and run.

You can fork https://github.com/electron/electron-quick-start and include a link to the branch with your changes.

If you provide a URL, please list the commands required to clone/setup/run your repo e.g.

  $ git clone $YOUR_URL -b $BRANCH
  $ npm install
  $ npm start || electron .

-->
```

If you believe that you have uncovered a bug in Electron, please fill out this
form to the best of your ability.

The two most important pieces of information we need in order to properly
evaluate the report is a description of the behavior you are seeing and a simple
test case we can use to recreate the problem on our own. If we cannot recreate
the issue, it becomes impossible for us to fix.

See [How to create a Minimal, Complete, and Verifiable example](https://stackoverflow.com/help/mcve).

## Triaging a Bug Report

Once an issue has been opened, it's not uncommon for there to be discussion
around it. Some contributors may have differing opinions about the issue,
including whether the behavior being seen is a bug or a feature. This discussion
is part of the process and should be kept focused, helpful, and professional.

Short, clipped responses—that provide neither additional context nor supporting
detail—are not helpful or professional. To many, such responses are simply
annoying and unfriendly.

Contributors are encouraged to help one another make forward progress as much
as possible, empowering one another to solve issues collaboratively. If you
choose to comment on an issue that you feel either is not a problem that needs
to be fixed, or if you encounter information in an issue that you feel is
incorrect, explain *why* you feel that way with additional supporting context,
and be willing to be convinced that you may be wrong. By doing so, we can often
reach the correct outcome much faster.

## Resolving a Bug Report

In the vast majority of cases, issues are resolved by opening a pull request.
The process for opening and reviewing a pull request is similar to that of
opening and triaging issues, but carries with it a necessary review and approval
workflow that ensures that the proposed changes meet the minimal quality and
functional guidelines of the Electron project.
