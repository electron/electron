## Purpose:
[GitHub Actions](https://docs.github.com/en/actions/learn-github-actions) gives you the flexibility to build an automated software development lifecycle workflow. With [GitHub Actions for Azure](https://docs.microsoft.com/en-gb/azure/developer/github/) you can create workflows that you can set up in your repository to build, test, package, release and deploy to Azure
This document is to set communication guidelines for making GitHub Actions ready for the external community. We want to set a robust framework for communication, and governance for anything related to GitHub Actions for Azure, for enabling both internal and external users to be always updated on any change in this area.

## Ground Rules for Communication:
1. Any form of communciation should abide by the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/)
2. We would use GitHub aliases for all the communication
3. We would want all communication to be public. 

## What should be communicated:
1. Anything that is relevant to external developers
2. Common utilities
3. Guidelines for contribution
4. Test and Test strategy
5. Versioning & Publishing Guidelines
6. Discussions related to issues & features
7. Roadmap & milestone planning

## What should not be be included in Open Source Communications:
1. Individual contact information or internal channel information (anything that is related to PII)
2. Internal processes that are not relevant or will not be used by external developers
3. Internal discussions and decisions on formulating open source guidelines
4. Any internal links that are not accessible to the community

## Entities involved:
#### 1. ACE team:
Team responsible for building infrastructure & platform, maintaining and advocating Actions for Azure. This is us. 
> a. DL - ace-team@github.com

> b. GitHub Team handles -GitHub org, Azure Org, MS Org
  
#### 2. ACE Steering Committee: 
Steering body for the Azure Actions. This should be ACE Team + key stakeholders from Azure & MS. Ex for all the key decision making for Azure Actions it would be ACE Team + Key Stakeholders from Azure.
> a) This can have folks from the MS team or community.

> b) Need to have a Teams channel for each org (for internal communication between ACE Team & MS Partner Team). Need github handles & MS email ids for these members

> c) Need Team handles for these - would be used for tagging in any communication with Azure org/MS org, code reviews etc

#### 3. Action Maintainer Group: 
For each Action, the Action maintainer group would include ACE Team + Owners of the specific Action. The owners would be decided by the team who creates the Action. 

## Communication scenarios:

| New Feature Req/Support Requests      | Create an issue in | Assignee| 
| ----------- | ----------- |------------|
| Related to existing GH Platform features      | Actions Repo       | ACE Team |
| Functionality, Security & Compliance of a specific Action   | The Specific Actions Repo        | Action Maintainer Group |
| Code of Conduct violation related to a specific Action  | Actions Repo | ACE Steering Committee | 
| Related to ACE Guidance and Libraries | Actions Repo | ACE Steering Committee |

## How will ACE Team communicate with the community/partner teams:
1. Roadmap & Milestones
2. Release notes to community which would include
> a. Breaking update or security related or regular update.

> b. New Action/New version Announcements 
3. Action Mantainers Group about :
> a. Breaking changes

> b. Security changes

> c.Regular updates

## Administrative scenarios (will be taken care by Team handles, no action item)
1. Employee leaving
2. Change in maintainer, escalation contact points
3. Change in ACE and steering team.
4. Change in ACE Team members

## Guiding principles
1. Communication open to the public is better than private only communication.
2. Communication should remain available even if people change.
3. Issues, discussion or chatops channels remain available.
4. Communication should be automated. Bots based communication have better acceptance.
