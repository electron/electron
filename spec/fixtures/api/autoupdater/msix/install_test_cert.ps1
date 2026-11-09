$ErrorActionPreference = 'Stop'
# Use this PowerShell installation's module, not one inherited from PowerShell 7.
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Security\Microsoft.PowerShell.Security.psd1') -ErrorAction Stop

# Path to cert file one folder up relative to script location
$scriptDir = Split-Path -Parent $PSCommandPath
$certPath = Join-Path $scriptDir "MSIXDevCert.cer" | Resolve-Path

# Load the certificate from file
$cert = [System.Security.Cryptography.X509Certificates.X509Certificate2]::new($certPath.Path)

$trustedStore = Get-ChildItem -Path "cert:\LocalMachine\TrustedPeople" | Where-Object { $_.Thumbprint -eq $cert.Thumbprint }
if (-not $trustedStore) {
    # We gonna need admin privileges to install the cert
    if (-Not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
      Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
      exit
    }
    # Install the public cert to LocalMachine\TrustedPeople (for MSIX trust)
    Import-Certificate -FilePath $certPath -CertStoreLocation "cert:\LocalMachine\TrustedPeople" | Out-Null
    Write-Host "  🏛️ Installed to: cert:\LocalMachine\TrustedPeople"
} else {
    Write-Host "  ✅ Certificate already trusted in: cert:\LocalMachine\TrustedPeople"
}