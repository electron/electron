dfe74f5
  "name": "@ci/CI.yml'@write-prettier.config":,'' :
  Skip to content
Search or jump to…
Pull requests
Issues
Codespaces
Marketplace
Explore
 
@mowjoejoejoejoe 
github
/
docs
Public
Fork your own copy of github/docs
Code
Issues
101
Pull requests
50
Discussions
Actions
Projects
4
Security
Insights
Add automated spell check #17674
jdkato started this conversation in Ideas
Add automated spell check
#17674
@jdkato
jdkato
on May 6, 2022 · 3 comments
Return to top
Discussion options

jdkato
on May 6, 2022
Hi,

I'm the maintainer of Vale, a linter for prose that allows users to implement custom quality-assurance checks (spell check, repeated words, capitalization, etc.) for their documentation.

One of the challenges of implementing Vale (especially with Markdown content) is getting it to correctly handle what I call "non-standard" markup: templating, shortcodes, snippets, and so on. To make this easier, I'm in the process of creating reusable configurations (or, "packages") for common static site generators and template engines, and I chose this repo as a test case for the Liquid package.

Findings
The following are a few of the errors I found while testing the package. I aired on the side of caution while going through the results; I ignored cases like "collectd" / "Collectd" (casing inconsistencies) and "filesize" / "file size" (spacing inconsistencies), among others.

You can find the full list of cases that I ignored below.

accept.txt (ignored cases)
Error	Correction	Location
from from	from	53:246
secert	secret	78:230
the the	the	45:256
be be	be	28:212
worflows	workflows	206:86
Luhasnk	Luhansk	51:45
transfering	transferring	535:154
Nothwithstanding	Notwithstanding	287:70
apprioriate	appropriate	121:283
currenly	currently	23:259
vulnerabilites	vulnerabilities	43:36
organizaton	organization	213:136
offical	official	29:39
Commiters	Committers	100:143
Configuration
If you'd like to add Vale to this repo, here's the configuration I used:

StylesPath = .github/styles
MinAlertLevel = suggestion

# The accept.txt file is given above
Vocab = Base

SkippedScopes = script, style, pre, figure, alt, strong

[*.md]
BasedOnStyles = Vale

TokenIgnores = ({%.+?%}), ({{.+?}})

BlockIgnores = (?s) *({% raw %}\n.*?\n{% endraw %})
This generally worked pretty good. There were still a few edge cases that I needed to disabled Vale on (using <!-- vale off --> comments), such as the tables in migrating-from-circleci-to-github-actions.md.

I understand if this isn't something you're interested in, but I figured I'd share my work just in case.

You can't vote on a locked discussion
sarahs reacted with heart emoji
Replies:3 comments
Comment options

ramyaparimi
on May 9, 2022
@jdkato Thanks so much for sharing your thoughts in here. We have been trying to implement Vale on this repo since quite some time. We faced a few blockers owing to the challenges while handling non standard markup situations. We raised raised this and this issue in the past with the challenges we had.

With your suggestion, I am currently using TokenIgnores to ignore liquids during linting. The use of regex in BlockedIgnores is something I haven't tried so I am going to check out your configuration.

Thanks so much for choosing the GitHub docs repo as a test for your use case. I will get back to you once I test this and discuss with the team 💖 .

You can't vote on a locked discussion
shukla2004 reacted with thumbs up emoji
0 replies
Comment options

sarahs
on May 24, 2022
Maintainer
Thanks @jdkato, this is so great! You're right that Liquid in our Markdown has posed some challenges for us in setting up Vale.

I tried out your config and the results are really promising. The main scenario I wasn't able to get it working on is nested Liquid. So for example, in this block:

1. Under "Management", type the username of the person you want to designate as a {% data variables.product.prodname_github_app %} manager in the organization, and click **Grant**. 

    ![Add a {% data variables.product.prodname_github_app %} manager](/assets/images/help/organizations/add-github-app-manager.png)
☝️ Vale correctly ignores the first data tag but incorrectly flags the term prodname in the second tag. I am assuming this is because the second tag is nested in image markup, although I am not 100% sure that's the reason.

One workaround we've had success with is simply stripping out the Liquid tags via a regex (a la perl -p -e 's/{%.*?%}//g' content/*) before running Vale. We realize this hack only works because our Liquid tags never span more than one line; if our replacement changed line breaks, the Vale action's PR commenting would be inaccurate. But the simplicity of this approach has an appeal.

Curious what you think about the nested tag scenario and the best approach for that.

You can't vote on a locked discussion
0 replies
Comment options

jdkato
on May 26, 2022
Author
The configuration I shared above should handle this case by adding alt to SkippedScopes. It's not ideal because you miss out on linting alt tags but, at the same time, the prevalence of templating in them means the content isn't all that meaningful from a linting perspective anyway.

You can't vote on a locked discussion
sarahs reacted with thumbs up emoji
0 replies
An owner of this repository has limited the ability to comment from new users.
Category
💡
Ideas
Labels
None yet
3 participants
@jdkato
@sarahs
@ramyaparimi
Notifications
You’re not receiving notifications from this thread.
 Create issue from discussion
Footer
© 2023 GitHub, Inc.
Footer navigation
Terms
Privacy
Security
Status
Docs
Contact GitHub
Pricing
API
Training
Blog
About
Add automated spell check · Discussion #17674 · github/docs
  "version": "0.0.0",
  "private": true,
  "license": "MIT",
  "dependencies": {
    "fs-extra": "^10.1.0",
    "js-yaml": "^4.1.0"
 https://github.com/chawchangping/nodecast/compare/master%40%7B1day%7D...masterSkip to content
Search or jump to…
Pull requests
Issues
Codespaces
Marketplace
Explore
 
@mowjoejoejoejoe 
mowjoejoejoejoe
/
WORKSFLOW
Public
Cannot fork because you own this repository and are not a member of any organizations.
Code
Issues
4
Pull requests
8
Actions
Projects
Wiki
Security
Insights
Settings
WORKSFLOW
/
Checkout via command line If the conflicts on this branch are too complex to resolve in the web editor, you can check it out via command line to resolve the conflicts. https:
/
github.com
/
mowjoejoejoejoe
/
WORKSFLOW.git Step 1: Clone the repository or update your local repository with the latest changes.  git pull origin main Step 2: Switch to the head branch of the pull request.  git checkout Master Step 3: Merge the base branch into the head branch.  git merge main Step 4: Fix the conflicts and commit the result.  See Resolving a merge conflict using the command line for step-by-step instructions on resolving merge conflicts. Step 5: Push the changes.  git push -u origin Master
in
bitore.sig
 

Spaces

2

No wrap
1
# Checkout via command line
2
If the conflicts on this branch are too complex to resolve in the web editor, you can check it out via command line to resolve the conflicts.
3
#https:www.bitore.net:
4
https://github.com/mowjoejoejoejoe/WORKSFLOW.git
5
Step 1: Clone the repository or update your local repository with the latest changes.
6
git pull origin main
7
Step 2: Switch to the head branch of the pull request.
8
git checkout Master
9
Step 3: Merge the base branch into the head branch.
10
git merge main
11
Step 4: Fix the conflicts and commit the result.
12
See Resolving a merge conflict using the command line for step-by-step instructions on resolving merge conflicts.
13
Step 5: Push the changes.
14
git push -u origin Master WORKSFLOW
15
 AUTOMATE AUTOMATES BEGIN GLOW4 AUTOMATES#Test :tests :tests :Run'@ci:
16 :'- '\commits :directly to the bitore.sig branch''
 '- '#'Create a new branch for this commit and start a pull request. Learn more about pull requests.
:Build::
