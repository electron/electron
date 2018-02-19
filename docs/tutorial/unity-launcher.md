# Unity Launcher Shortcuts

In Unity, you can add custom entries to its launcher via modifying the
`.desktop` file, see [Adding Shortcuts to a Launcher][unity-launcher].

__Launcher shortcuts of Audacious:__

![audacious][audacious-launcher]

Generally speaking, shortcuts are added by providing a `Name` and `Exec`
property for each entry in the shortcuts menu. Unity will execute the
`Exec` field once clicked by the user. The format is as follows:

```text
Actions=PlayPause;Next;Previous

[Desktop Action PlayPause]
Name=Play-Pause
Exec=audacious -t
OnlyShowIn=Unity;

[Desktop Action Next]
Name=Next
Exec=audacious -f
OnlyShowIn=Unity;

[Desktop Action Previous]
Name=Previous
Exec=audacious -r
OnlyShowIn=Unity;
```

Unity's preferred way of telling your application what to do is to use
parameters. You can find these in your app in the global variable
`process.argv`.

[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[audacious-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png
