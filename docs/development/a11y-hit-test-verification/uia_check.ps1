# Controleur UIA (Windows) du hit-test d'accessibilite d'Electron.
#
# Objectif : mesurer ce que voit un lecteur d'ecran au point de la souris.
# On interroge l'API Windows UI Automation (la meme couche qu'utilise NVDA
# pour le suivi de souris et qu'inspecte Inspect.exe), exactement comme le
# rapport du bug electron/electron#42945 le decrit.
#
# Usage : powershell.exe -ExecutionPolicy Bypass -File uia_check.ps1 -Json <chemin>\window.json [-Arbre]
#
# Sortie : texte simple, une ligne par resultat, lisible au lecteur d'ecran.

param(
  [Parameter(Mandatory = $true)][string]$Json,
  [switch]$Arbre
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName WindowsBase

$AE = [System.Windows.Automation.AutomationElement]
$TS = [System.Windows.Automation.TreeScope]

if (-not (Test-Path $Json)) { Write-Output "ERREUR: fichier introuvable: $Json"; exit 1 }
$info = Get-Content -Raw -Encoding UTF8 $Json | ConvertFrom-Json

$proc = Get-Process -Id $info.pid -ErrorAction Stop
$handle = $proc.MainWindowHandle
if ($handle -eq 0) { Write-Output "ERREUR: aucune fenetre pour le processus $($info.pid)"; exit 1 }

$fenetre = $AE::FromHandle($handle)
$rect = $fenetre.Current.BoundingRectangle

Write-Output ("FENETRE pid={0} titre=""{1}"" rect={2},{3} {4}x{5}" -f `
  $info.pid, $fenetre.Current.Name, [int]$rect.X, [int]$rect.Y, [int]$rect.Width, [int]$rect.Height)
Write-Output ''

function Describe($element) {
  if ($null -eq $element) { return 'AUCUN ELEMENT' }
  $nom = $element.Current.Name
  if ([string]::IsNullOrEmpty($nom)) { $nom = '(sans nom)' }
  $type = $element.Current.ControlType.ProgrammaticName -replace '^ControlType\.', ''
  $classe = $element.Current.ClassName
  $aid = $element.Current.AutomationId
  return ("nom=""{0}"" type={1} classe={2} automationId={3}" -f $nom, $type, $classe, $aid)
}

function ChaineDesAncestres($element) {
  $morceaux = @()
  $courant = $element
  $garde = 0
  while ($null -ne $courant -and $garde -lt 12) {
    $morceaux += (Describe $courant)
    $courant = [System.Windows.Automation.TreeWalker]::ControlViewWalker.GetParent($courant)
    $garde++
  }
  return $morceaux
}

Write-Output '=== HIT-TEST (ElementFromPoint) ==='
foreach ($point in $info.points) {
  $x = [int]$rect.X + [int]$point.x
  $y = [int]$rect.Y + [int]$point.y
  $hit = $AE::FromPoint((New-Object System.Windows.Point $x, $y))
  Write-Output ''
  Write-Output ("-- point {0} (relatif {1},{2} / ecran {3},{4})" -f $point.nom, $point.x, $point.y, $x, $y)
  Write-Output ("   ATTEINT : " + (Describe $hit))
  $chain = ChaineDesAncestres $hit
  $i = 0
  foreach ($maillon in $chain) {
    Write-Output ("   ascendant[{0}] : {1}" -f $i, $maillon)
    $i++
  }
}

if ($Arbre) {
  Write-Output ''
  Write-Output '=== ARBRE D ACCESSIBILITE DE LA FENETRE (profondeur 8) ==='
  function AfficherArbre($element, $profondeur) {
    if ($profondeur -gt 8) { return }
    $condition = New-Object System.Windows.Automation.PropertyCondition `
      ([System.Windows.Automation.AutomationElement]::ControlTypeProperty), `
      ([System.Windows.Automation.ControlType]::Group)
    $condition2 = New-Object System.Windows.Automation.PropertyCondition `
      ([System.Windows.Automation.AutomationElement]::ControlTypeProperty), `
      ([System.Windows.Automation.ControlType]::Pane)
    $condition3 = New-Object System.Windows.Automation.PropertyCondition `
      ([System.Windows.Automation.AutomationElement]::ControlTypeProperty), `
      ([System.Windows.Automation.ControlType]::Document)
    $enfants = $element.FindAll($TS::Children, [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($enfant in $enfants) {
      $indent = ' ' * ($profondeur * 2)
      Write-Output ("{0}{1}" -f $indent, (Describe $enfant))
      AfficherArbre $enfant ($profondeur + 1)
    }
  }
  AfficherArbre $fenetre 0
}
