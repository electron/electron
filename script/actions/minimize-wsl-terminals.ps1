# The windows-11-arm image keeps relaunching `wsl.exe --update`, each time in
# a new terminal window that takes the foreground over the test area, so
# Chromium's native occlusion tracking reports Electron's windows beneath it
# as hidden. Minimize each one as it appears and log it with its parent
# process. Runs in Windows PowerShell 5.1 so the C# compiles with the in-box
# .NET Framework compiler.
param(
  [Parameter(Mandatory = $true)][string]$Log,
  [int]$Seconds = 3600
)
Add-Type -TypeDefinition (Get-Content -Raw (Join-Path $PSScriptRoot 'minimize-wsl-terminals.cs'))
[WslTerminalMinimizer]::Run($Log, $Seconds)
