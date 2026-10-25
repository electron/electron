# Vérification du hit-test d'accessibilité d'une fenêtre Electron (Windows)

Ce dossier décrit comment vérifier, sur Windows, que le pointeur de la souris
n'atterrit pas sur une « pane » vide au lieu du contenu web de la fenêtre.

## Pourquoi deux chemins sont mesurés

Un même point de l'écran peut être interprété de deux façons par Windows :

- **UIA** (UI Automation) : `AutomationElement.FromPoint`.
- **MSAA** (IAccessible) : `AccessibleObjectFromPoint`, dans `oleacc.dll`.

Le second est le chemin hérité, celui qu'utilise le mode hérité de NVDA pour les
applications Electron, et celui qu'affiche Inspect.exe en mode MSAA. C'est ce
chemin qui était bloqué par la `contentView` vide placée au-dessus de la
`WebContentsView` (electron/electron#42945).

## Fichiers

- `app.js` : application de test. Elle ouvre une fenêtre, y place des textes à
  positions fixes, ajoute une vue enfant de 40 x 40 dans la `contentView`, et
  écrit un fichier de mesures dans le répertoire temporaire.
- `uia_check.ps1` : contrôleur. Il interroge les deux chemins aux points de
  contrôle et écrit le résultat sous forme de texte, une ligne par résultat.

## Marche à suivre

1. Compiler Electron, ou utiliser une version publiée.
2. Lancer l'application de test avec le binaire à vérifier :

   ```
   <electron.exe> <dossier de ce fichier>
   ```

3. Dans une autre fenêtre, interroger les points de contrôle :

   ```
   powershell.exe -ExecutionPolicy Bypass -File uia_check.ps1 -Json %TEMP%\electron-a11y-hit-test.json
   ```

Le fichier de mesures contient la position de la zone de contenu de la
fenêtre, le facteur d'échelle de l'écran et la position des points de contrôle,
en pixels logiques. Le contrôleur applique le facteur d'échelle, parce que
l'API d'accessibilité travaille en pixels physiques.

## Résultat attendu

- **Avant le correctif** : le chemin MSAA renvoie `role=16`
  (`ROLE_SYSTEM_PANE`), sans nom, à tous les points de texte. C'est la
  « pane » vide : un lecteur d'écran en mode hérité n'y trouve rien à dire.
- **Après le correctif** : le chemin MSAA renvoie le texte sous le pointeur.

Le chemin UIA renvoie le document web dans les deux cas : il n'était pas
concerné par le défaut.