param([string]$gomaDir=$PWD)
$cmdPath = Join-Path -Path $gomaDir -ChildPath "goma_ctl.py"
Start-Process -FilePath cmd -ArgumentList "/C", "python3", "$cmdPath", "ensure_start"
$timedOut = $false; $waitTime = 0; $waitIncrement = 5; $timeout=120;
Do { sleep $waitIncrement; $timedOut = (($waitTime+=$waitIncrement) -gt $timeout); iex "$gomaDir\gomacc.exe port 2" > $null; } Until(($LASTEXITCODE -eq 0) -or $timedOut)
if ($timedOut) {
    write-error 'Timed out waiting for goma to start'; exit 1;
} else {
    Write-Output "Successfully started goma!"
}
