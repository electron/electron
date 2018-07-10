$scriptPath = split-path -parent $MyInvocation.MyCommand.Definition
& python "$scriptPath/cibuild"
