# Task Object

* `program` string - Path of the program to execute, usually you should
  specify `process.execPath` which opens the current program.
* `arguments` string - The command line arguments when `program` is
  executed.
* `title` string - The string to be displayed in a JumpList.
* `description` string - Description of this task.
* `iconPath` string - The absolute path to an icon to be displayed in a
  JumpList, which can be an arbitrary resource file that contains an icon. You
  can usually specify `process.execPath` to show the icon of the program.
* `iconIndex` number - The icon index in the icon file. If an icon file
  consists of two or more icons, set this value to identify the icon. If an
  icon file consists of one icon, this value is 0.
* `workingDirectory` string (optional) - The working directory. Default is empty.
