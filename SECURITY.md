# Reporting Security Issues

The Electron team and community take security bugs in Electron seriously. We appreciate your efforts to responsibly disclose your findings, and will make every effort to acknowledge your contributions.

To report a security issue, please use the GitHub Security Advisory ["Report a Vulnerability"](https://github.com/electron/electron/security/advisories/new) tab.

The Electron team will send a response indicating the next steps in handling your report. After the initial reply to your report, the security team will keep you informed of the progress towards a fix and full announcement, and may ask for additional information or guidance.

Report security bugs in third-party modules to the person or team maintaining the module. You can also report a vulnerability through the [npm contact form](https://www.npmjs.com/support) by selecting "I'm reporting a security vulnerability".

## Escalation

If you do not receive an acknowledgement of your report within 6 business days, or if you cannot find a private security contact for the project, you may escalate to the OpenJS Foundation CNA at `security@lists.openjsf.org`.

If the project acknowledges your report but does not provide any further response or engagement within 14 days, escalation is also appropriate.

## The Electron Security Notification Process

For context on Electron's security notification process, please see the [Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md#notifications) section of the Security WG's [Membership and Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md) Governance document.

## Learning More About Security

To learn more about securing an Electron application, please see the [security tutorial](docs/tutorial/security.md).
# Creating a default community health file

You can create default community health files, such as CONTRIBUTING and CODE_OF_CONDUCT. Default files will be used for any repository owned by the account that does not contain its own file of that type.

## About default community health files

Default community health files are a set of predefined files that provide guidance and templates for maintaining a healthy and collaborative open source project. These files help you automate and standardize various aspects of your project's development and community interaction, promoting transparency, good practices, and collaboration.

You can add default community health files to a repository called `.github`. The `.github` repository must be **public**.

GitHub will use and display default files for any repository owned by the account, regardless of the destination repository's visibility, that does not have its own file of that type. For supported files that can be stored in more than one location, GitHub uses the following order of precedence:

* The `.github` folder
* The root of the repository
* The `docs` folder

If no corresponding file is found in the current repository, GitHub will use the default file from the `.github` repository. For files that can be stored in more than one location, GitHub follows the same order of precedence in the `.github` repository.

For example, anyone who creates an issue or pull request in a repository that does not have its own `CONTRIBUTING.md` file will see a link to the default `CONTRIBUTING.md` from the `.github` repository. However, if a repository defines valid issue templates or issue template configuration in its own `.github/ISSUE_TEMPLATE` folder, none of the contents of the default `.github/ISSUE_TEMPLATE` folder will be used. This allows repository maintainers to override the default files with specific templates or content on a per-repository basis.

Storing the files in `.github` repository allows making changes to the defaults just in one place. Additionally, they won’t appear in the file browser or Git history of the individual repositories, and are not included in their clones, packages, or downloads.

As a repository maintainer, you can use the community standards checklist to see if your project meets the recommended community standards to help people use and contribute to your project. For more information, see [About community profiles for public repositories](/en/communities/setting-up-your-project-for-healthy-contributions/about-community-profiles-for-public-repositories).

## About security policies

After someone reports a security vulnerability in your project, you can use GitHub Security Advisories to disclose, fix, and publish information about the vulnerability. For more information about the process of reporting and disclosing vulnerabilities in GitHub, see [Coordinated disclosure of security vulnerabilities](/en/code-security/concepts/vulnerability-reporting-and-management/coordinated-disclosure#about-reporting-and-disclosing-vulnerabilities-in-projects-on-github). For more information about repository security advisories, see [Repository security advisories](/en/code-security/concepts/vulnerability-reporting-and-management/repository-security-advisories).

For an example of a real `SECURITY.md` file, see <https://github.com/electron/electron/blob/main/SECURITY.md>.

## Supported file types

You can create defaults in your organization or personal account for the following community health files:

| Community health file                             | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| ------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| *CODE\_OF\_CONDUCT.md*                            | A CODE\_OF\_CONDUCT file defines standards for how to engage in a community. For more information, see [Adding a code of conduct to your project](/en/communities/setting-up-your-project-for-healthy-contributions/adding-a-code-of-conduct-to-your-project).                                                                                                                                                                                                                                                                 |
| *CONTRIBUTING.md*                                 | A CONTRIBUTING file communicates how people should contribute to your project. For more information, see [Setting guidelines for repository contributors](/en/communities/setting-up-your-project-for-healthy-contributions/setting-guidelines-for-repository-contributors).                                                                                                                                                                                                                                                   |
| Discussion category forms                         | Discussion category forms customize the templates that are available for community members to use when they open new discussions in your repository. For more information, see [Creating discussion category forms](/en/discussions/managing-discussions-for-your-community/creating-discussion-category-forms).                                                                                                                                                                                                               |
|                                                   |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| *FUNDING.yml*                                     | A FUNDING file displays a sponsor button in your repository to increase the visibility of funding options for your open source project. For more information, see [Displaying a sponsor button in your repository](/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/displaying-a-sponsor-button-in-your-repository).                                                                                                                                                               |
|                                                   |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| Issue and pull request templates and *config.yml* | Issue and pull request templates customize and standardize the information you'd like contributors to include when they open issues and pull requests in your repository. For more information, see [About issue and pull request templates](/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests/about-issue-and-pull-request-templates).<br /><br />If an issue template sets a label, that label must be created in your `.github` repository and any repositories where the template will be used. |
| *SECURITY.md*                                     | A SECURITY file gives instructions on how to report a security vulnerability in your project and description that hyperlinks the file. For more information, see [Adding a security policy to your repository](/en/code-security/how-tos/report-and-fix-vulnerabilities/configure-vulnerability-reporting/add-security-policy).                                                                                                                                                                                                |
| *SUPPORT.md*                                      | A SUPPORT file lets people know about ways to get help with your project. For more information, see [Adding support resources to your project](/en/communities/setting-up-your-project-for-healthy-contributions/adding-support-resources-to-your-project).                                                                                                                                                                                                                                                                    |

You cannot create a default license file. License files must be added to individual repositories so the file will be included when a project is cloned, packaged, or downloaded.

## Creating a repository for default files

1. In the upper-right corner of any page, select <svg version="1.1" width="16" height="16" viewBox="0 0 16 16" class="octicon octicon-plus" aria-label="Create something new" role="img"><path d="M7.75 2a.75.75 0 0 1 .75.75V7h4.25a.75.75 0 0 1 0 1.5H8.5v4.25a.75.75 0 0 1-1.5 0V8.5H2.75a.75.75 0 0 1 0-1.5H7V2.75A.75.75 0 0 1 7.75 2Z"></path></svg>, then click **New repository**.

   ![Screenshot of a GitHub dropdown menu showing options to create new items. The menu item "New repository" is outlined in dark orange.](/assets/images/help/repository/repo-create-global-nav-update.png)
2. Use the **Owner** drop-down menu, and select the organization or personal account you want to create default files for.
   ![Screenshot of the owner menu for a new GitHub repository. The menu shows two options, octocat and github.](/assets/images/help/repository/create-repository-owner.png)
3. In the "Repository name" field, type **.github**.
4. Optionally, in the "Description" field, type a description.
5. Make sure the repository status is set to **Public**. A repository for default files cannot be private.
6. Toggle **Add README** to **On**.
7. Click **Create repository**.
8. In the repository, create one of the supported community health files. Discussion category forms must be in a folder called `.github/DISCUSSION_TEMPLATE`. Issue templates and their configuration file must be in a folder called `.github/ISSUE_TEMPLATE`. A `FUNDING.yml` file must be in the `.github` folder. All other supported files may be in the root of the repository, the `.github` folder, or the `docs` folder. For more information, see [Creating new files](/en/repositories/working-with-files/managing-files/creating-new-files)./**
 * Copyright © 2026 徐嘉糧 (GUBON LUCID OS / GUBON-EX). All rights reserved.
 *
 * 中文：
 * 本系統之原始碼、系統架構、軟體設計、演算法邏輯、資料結構、
 * 私有化簽章與驗證機制，以及相關商業流程與閉環設計，
 * 其依法可受保護之權利，除另有明確書面約定外，均由權利人享有。
 *
 * English:
 * The source code, system architecture, software design, algorithmic logic,
 * data structures, sovereign signing and verification mechanisms, and
 * related commercial workflows and closed-loop designs of this system,
 * together with all rights legally protectable therein, are owned by the
 * rights holder unless otherwise expressly agreed in writing.
 *
 * Unauthorized reproduction, distribution, modification, disclosure,
 * sublicensing, or deployment is prohibited to the extent permitted by law.
 */
# Intellectual Property & Sovereign Notice

Copyright © 2026 徐嘉糧  
GUBON LUCID OS / GUBON-EX  
All rights reserved.

## 中文

除另有明確書面授權或契約約定外，GUBON LUCID OS 及 GUBON-EX 所涉及之原始碼、軟體架構、系統設計、演算法與程式邏輯、資料結構、私有化簽章及驗證機制、商業流程、決策流程、產品設計及相關技術文件，其依法可受保護之智慧財產權及其他權利均由權利人享有。

未經適當授權，任何人不得對受保護內容進行未經授權之複製、重製、修改、散布、公開傳輸、轉讓、再授權、商業利用或部署。

本聲明不影響第三方軟體、開源元件、API、SDK、模型、服務或其他內容所適用之原有授權條款。

---

## English

Unless expressly licensed or otherwise agreed in writing, all legally protectable intellectual property and other rights relating to GUBON LUCID OS and GUBON-EX, including source code, software architecture, system design, algorithms and program logic, data structures, sovereign signing and verification mechanisms, commercial workflows, decision processes, product designs, and related technical documentation, are owned by the rights holder.

Without appropriate authorization, no person may reproduce, modify, distribute, publicly communicate, transfer, sublicense, commercially exploit, or deploy protected materials.

Nothing in this notice overrides the applicable license terms of third-party software, open-source components, APIs, SDKs, models, services, or other third-party materials.

# Third-Party & Open Source Notices

GUBON LUCID OS / GUBON-EX incorporates various open-source software, third-party libraries, APIs, and SDKs (including but not limited to Node.js, React, Next.js, PostgreSQL, Prisma, Redis, BullMQ, and payment SDKs such as PayPal REST API v2). 

Each third-party component remains subject to its respective original license terms (e.g., MIT, Apache 2.0, ISC). Nothing in the GUBON-EX proprietary licensing structure alters, supersedes, or restricts the rights and obligations granted under those respective open-source or third-party licenses.

For detailed third-party dependency licenses, please refer to the respective package manifests (`package.json`) and upstream documentation.
# Intellectual Property & Sovereign Notice

Copyright © 2026 徐嘉糧
GUBON LUCID OS / GUBON-EX
All rights reserved.

## 中文

除另有明確書面授權或契約約定外，GUBON LUCID OS
及 GUBON-EX 所涉及之原始碼、軟體架構、系統設計、
演算法與程式邏輯、資料結構、私有化簽章及驗證機制、
商業流程、決策流程、產品設計及相關技術文件，
其依法可受保護之智慧財產權及其他權利均由權利人享有。

未經適當授權，任何人不得對受保護內容進行未經授權之
複製、重製、修改、散布、公開傳輸、轉讓、再授權、
商業利用或部署。

本聲明不影響第三方軟體、開源元件、API、SDK、模型、
服務或其他內容所適用之原有授權條款。

## English

Unless expressly licensed or otherwise agreed in writing,
all legally protectable intellectual property and other rights
relating to GUBON LUCID OS and GUBON-EX, including source code,
software architecture, system design, algorithms and program logic,
data structures, sovereign signing and verification mechanisms,
commercial workflows, decision processes, product designs,
and related technical documentation, are owned by the rights holder.

Without appropriate authorization, no person may reproduce,
modify, distribute, publicly communicate, transfer, sublicense,
commercially exploit, or deploy protected materials.

Nothing in this notice overrides the applicable license terms
of third-party software, open-source components, APIs, SDKs,
models, services, or other third-party materials.
