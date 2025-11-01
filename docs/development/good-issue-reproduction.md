# Why You Must Provide a Minimal Reproduction Through Fiddle

A clear, minimal reproduction helps maintainers quickly identify and resolve your issue.  
This guide explains **why** it’s required and **how** to create one using [Electron Fiddle](https://www.electronjs.org/fiddle).

---

## Introduction

The Electron team receives hundreds of issues each month.  
To manage and resolve them efficiently, we rely on **minimal, easily reproducible examples** created using Electron Fiddle.  
This document explains why this requirement exists and how you can create an effective reproduction for your issue.

---

## Why Provide a Good Reproduction?

### 1. Respect the Maintainers’ Time
Electron maintainers are busy reviewing, debugging, and fixing issues.  
If you don’t supply a simple, runnable reproduction, we often don’t have the bandwidth to investigate your problem.  
The easier you make it for maintainers to reproduce your issue, the higher the chances that it will be resolved.

### 2. Increase the Likelihood of Your Issue Being Fixed
A clear, minimal reproduction removes ambiguity.  
Maintainers can immediately test your issue and understand exactly what’s going wrong.  
The faster we can reproduce a bug, the faster we can fix it.

### 3. Discover Issues in Your Own Code
When you simplify your problem into a minimal example, you might find that the bug originates from your own code rather than Electron itself.  
This saves you time and gets you a solution faster.

### 4. Security and Safety
Cloning arbitrary repositories can expose maintainers to security vulnerabilities or malicious code.  
A Fiddle reproduction ensures a safe, contained, and trusted environment for everyone.

### 5. Shared Understanding
A Fiddle reproduction ensures everyone—maintainers and contributors—are discussing the same issue in the same context.

---

## What Is a Good Reproduction?

A good reproduction includes:
- Only the **minimum code necessary** to reproduce the issue.  
  Remove everything not essential to demonstrating the bug.
- A **simple, self-contained Electron Fiddle** link.
- Clear steps to reproduce the issue.
- No dependencies on external services or complex setups.

### Bad Reproductions
- Large GitHub repositories with unrelated code.  
- Code snippets that cannot be executed standalone.  
- Descriptions without any runnable example.

⚠️ **Note:** Issues that do not provide a reproducible Fiddle example will be **closed**.

---

## How to Create a Fiddle Reproduction

1. Open [Electron Fiddle](https://www.electronjs.org/fiddle).  
2. Reproduce your issue with the minimal possible code.  
3. Click **“Save & Share”**.  
4. Copy the link and include it in your GitHub issue.

---

## FAQ: Common Objections

**“My reproduction is simple enough.”**  
If it’s simple enough, creating a minimal Fiddle should take less than five minutes.  
Doing so ensures everyone sees exactly what you see.

**“You’re the maintainers—you should build the repro.”**  
We prioritize issues with clear reproductions.  
Expecting maintainers to build one for every report is not scalable.  
Providing a minimal repro is part of collaborating effectively in an open-source project.

**“The issue only happens in my app.”**  
That usually means it’s environment-specific.  
Simplifying the problem often uncovers what causes it.

**“I can’t reproduce it outside my large project.”**  
Try to remove unrelated code step-by-step until you isolate the issue.  
The process often identifies the real cause.

---

## Summary of Expectations

✅ Provide a **minimal Electron Fiddle link**  
✅ Ensure it runs without external setup  
✅ Include clear steps to reproduce the issue  
❌ Avoid large repos or incomplete snippets  
❌ Don’t expect maintainers to recreate your environment  

---

## Where This Will Be Used

- In the issue template as a comment.  
- In the automated message sent when the `blocked/needs-repro` label is added.  
- As a link maintainers can share when users resist creating a minimal reproduction.

---

## Alternatives Considered

1. **Maintainers continue explaining this repeatedly.**  
   This is inefficient and leads to inconsistent explanations.  
   A single comprehensive document saves everyone time.

2. **Relying on the bot message alone.**  
   Automated messages help but often trigger debate.  
   Linking to this document provides a definitive explanation that ends the discussion.

---

## References and Inspiration

- Inspired by community discussions and issues: #44264, #44690, #45251  
- [Levels.io Contact Page (archived version)](https://web.archive.org/web/20241116010936/https://levels.io/contact/)

---

*Thank you for helping us make the Electron project better and more maintainable by providing clear, minimal reproductions through Fiddle.*
