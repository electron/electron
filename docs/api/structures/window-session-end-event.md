# WindowSessionEndEvent Object extends `Event`

* `reasons` string[] - List of reasons for shutdown. Can be 'shutdown', 'close-app', 'critical', or 'logoff'.

Unfortunately, Windows does not offer a way to differentiate between a shutdown and a reboot, meaning the 'shutdown'
reason is triggered in both scenarios. For more details on the `WM_ENDSESSION` message and its associated reasons,
refer to the [MSDN documentation](https://learn.microsoft.com/en-us/windows/win32/shutdown/wm-endsession).
