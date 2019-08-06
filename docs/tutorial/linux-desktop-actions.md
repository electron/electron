# Custom Linux Desktop Launcher Actions

On many Linux environments, you can add custom entries to its launcher
by modifying the `.desktop` file. For Canonical's Unity documentation,
see [Adding Shortcuts to a Launcher][unity-launcher]. For details on a
more generic implementation, see the [freedesktop.org Specification][spec].

__Launcher shortcuts of Audacious:__

![audacious][audacious-launcher]

Generally speaking, shortcuts are added by providing a `Name` and `Exec`
property for each entry in the shortcuts menu. Unity will execute the
`Exec` field once clicked by the user. The format is as follows:

```plaintext
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
[spec]: https://specifications.freedesktop.org/desktop-entry-spec/1.1/ar01s11.html
