---
title: Desktop Launcher Actions
description: Add actions to the system launcher on Linux environments.
slug: linux-desktop-actions
hide_title: true
---

# Desktop Launcher Actions

## Overview

On many Linux environments, you can add custom entries to the system launcher
by modifying the `.desktop` file. For details, see the
[freedesktop.org Desktop Entry Specification][spec].

To create a shortcut, you need to provide `Name` and `Exec` properties for the
entry you want to add to the shortcut menu. The desktop will execute the
command defined in the `Exec` field after the user clicks the shortcut menu
item. An example of the `.desktop` file may look as follows:

```plaintext
Actions=PlayPause;Next;Previous

[Desktop Action PlayPause]
Name=Play-Pause
Exec=audacious -t

[Desktop Action Next]
Name=Next
Exec=audacious -f

[Desktop Action Previous]
Name=Previous
Exec=audacious -r
```

The preferred way for the desktop to instruct your application on what to do
is using parameters. You can find them in your application in the global
variable `process.argv`.

[spec]: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
