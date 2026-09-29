# Diagnostic for Windows CI: logs, whenever it changes, the foreground window
# and the windows able to occlude Electron's (per Chromium's native occlusion
# tracker filter) that sit above the topmost Electron window, plus display
# on/off and session lock changes. Runs in Windows PowerShell 5.1 so the C#
# compiles with the in-box .NET Framework compiler.
param(
  [Parameter(Mandatory = $true)][string]$Log,
  [int]$Seconds = 3600
)
Add-Type -TypeDefinition (Get-Content -Raw (Join-Path $PSScriptRoot 'occlusion-sampler.cs'))
[OcclusionSampler]::Run($Log, $Seconds)
