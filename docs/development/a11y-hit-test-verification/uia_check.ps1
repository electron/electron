# Controleur du hit-test d'accessibilite d'une fenetre Electron sous Windows.
#
# Deux chemins sont interroges, parce qu'ils ne se comportent pas de la meme
# facon et que le bug electron/electron#42945 ne concerne que le second :
#
# 1. UIA  : AutomationElement.FromPoint (API Windows UI Automation).
# 2. MSAA : AccessibleObjectFromPoint (oleacc.dll), c'est-a-dire le chemin
#            herite qu'utilise NVDA pour les applications Electron et que
#            Inspect.exe affiche en mode MSAA.
#
# Usage : powershell.exe -ExecutionPolicy Bypass -File uia_check.ps1 -Json <fichier>
#
# Le fichier de mesures est produit par app.js. Les coordonnees y sont en
# pixels logiques (DIP) : le facteur d'echelle de l'ecran est applique ici,
# car l'API d'accessibilite travaille en pixels physiques.
#
# Sortie : texte simple, une ligne par resultat, lisible au lecteur d'ecran.

param(
  [Parameter(Mandatory = $true)][string]$Json,
  [switch]$Arbre
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName Accessibility

# Appel direct de oleacc.dll pour la chaine MSAA.
if (-not ("OleAccNative" -as [type])) {
  Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class OleAccNative {
  [StructLayout(LayoutKind.Sequential)]
  public struct POINT { public int x; public int y; }

  [DllImport("oleacc.dll")]
  private static extern int AccessibleObjectFromPoint(POINT pt,
      [MarshalAs(UnmanagedType.Interface)] out Accessibility.IAccessible ppaccessible);

  // Retourne l'objet accessible sous le point donne, et le code HRESULT.
  public static Accessibility.IAccessible ObjectAt(int x, int y, out int code) {
    POINT pt = new POINT();
    pt.x = x;
    pt.y = y;
    Accessibility.IAccessible result;
    code = AccessibleObjectFromPoint(pt, out result);
    return result;
  }
}
"@ -ReferencedAssemblies Accessibility
}

$AE = [System.Windows.Automation.AutomationElement]
$TS = [System.Windows.Automation.TreeScope]

if (-not (Test-Path $Json)) { Write-Output "ERREUR: fichier introuvable: $Json"; exit 1 }
$info = Get-Content -Raw -Encoding UTF8 $Json | ConvertFrom-Json

$proc = Get-Process -Id $info.pid -ErrorAction Stop
$handle = $proc.MainWindowHandle
if ($handle -eq 0) { Write-Output "ERREUR: aucune fenetre pour le processus $($info.pid)"; exit 1 }

$fenetre = $AE::FromHandle($handle)
$rect = $fenetre.Current.BoundingRectangle
$echelle = [double]$info.echelle

Write-Output ("FENETRE pid={0} titre=""{1}"" rect={2},{3} {4}x{5} echelle={6}" -f `
  $info.pid, $fenetre.Current.Name, [int]$rect.X, [int]$rect.Y, [int]$rect.Width, [int]$rect.Height, $echelle)
Write-Output ''

function DescribeMsaa($accessible) {
  if ($null -eq $accessible) { return 'AUCUN OBJET' }
  try {
    $nom = $accessible.accName(0)
    $role = $accessible.accRole(0)
    $enfants = $accessible.accChildCount
  } catch {
    return "ERREUR DE LECTURE: " + $_.Exception.Message
  }
  if ([string]::IsNullOrEmpty($nom)) { $nom = '(sans nom)' }
  $texte = ("nom=""{0}"" role={1} enfants={2}" -f $nom, $role, $enfants)
  # Detail du premier enfant : sans lui, on ne voit pas ce que contient
  # realmente la pane vide.
  try {
    if ($enfants -gt 0) {
      $enfant = $accessible.get_accChild(1)
      $nomEnfant = $enfant.accName(0)
      $roleEnfant = $enfant.accRole(0)
      if ([string]::IsNullOrEmpty($nomEnfant)) { $nomEnfant = '(sans nom)' }
      $texte += (" | enfant 1 : nom=""{0}"" role={1}" -f $nomEnfant, $roleEnfant)
    }
  } catch {
    $texte += ' | enfant 1 : illisible'
  }
  return $texte
}

function DescribeUia($element) {
  if ($null -eq $element) { return 'AUCUN ELEMENT' }
  $nom = $element.Current.Name
  if ([string]::IsNullOrEmpty($nom)) { $nom = '(sans nom)' }
  $type = $element.Current.ControlType.ProgrammaticName -replace '^ControlType\.', ''
  return ("nom=""{0}"" type={1} classe={2}" -f $nom, $type, $element.Current.ClassName)
}

function ChaineDesAncestres($element) {
  $morceaux = @()
  $courant = $element
  $garde = 0
  while ($null -ne $courant -and $garde -lt 12) {
    $morceaux += (DescribeUia $courant)
    $courant = [System.Windows.Automation.TreeWalker]::ControlViewWalker.GetParent($courant)
    $garde++
  }
  return $morceaux
}

Write-Output '=== HIT-TEST UIA (AutomationElement.FromPoint) ==='
foreach ($point in $info.points) {
  $x = [int][math]::Round(([double]$info.bornes.x + [double]$point.x) * $echelle)
  $y = [int][math]::Round(([double]$info.bornes.y + [double]$point.y) * $echelle)
  $hit = $AE::FromPoint((New-Object System.Windows.Point $x, $y))
  Write-Output ''
  Write-Output ("-- point {0} (relatif {1},{2} / ecran {3},{4})" -f $point.nom, $point.x, $point.y, $x, $y)
  Write-Output ("   ATTEINT : " + (DescribeUia $hit))
  $chain = ChaineDesAncestres $hit
  $i = 0
  foreach ($maillon in $chain) {
    Write-Output ("   ascendant[{0}] : {1}" -f $i, $maillon)
    $i++
  }
}

Write-Output ''
Write-Output '=== HIT-TEST MSAA (AccessibleObjectFromPoint) ==='
foreach ($point in $info.points) {
  $x = [int][math]::Round(([double]$info.bornes.x + [double]$point.x) * $echelle)
  $y = [int][math]::Round(([double]$info.bornes.y + [double]$point.y) * $echelle)
  $code = 0
  $accessible = [OleAccNative]::ObjectAt($x, $y, [ref]$code)
  Write-Output ''
  Write-Output ("-- point {0} (relatif {1},{2} / ecran {3},{4})" -f $point.nom, $point.x, $point.y, $x, $y)
  Write-Output ("   code retour : {0}" -f $code)
  Write-Output ("   ATTEINT : " + (DescribeMsaa $accessible))
}

if ($Arbre) {
  Write-Output ''
  Write-Output '=== ARBRE D ACCESSIBILITE DE LA FENETRE (UIA, profondeur 6) ==='
  function AfficherArbre($element, $profondeur) {
    if ($profondeur -gt 6) { return }
    $enfants = $element.FindAll($TS::Children, [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($enfant in $enfants) {
      $indent = ' ' * ($profondeur * 2)
      Write-Output ("{0}{1}" -f $indent, (DescribeUia $enfant))
      AfficherArbre $enfant ($profondeur + 1)
    }
  }
  AfficherArbre $fenetre 0
}
