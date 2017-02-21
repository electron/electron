# Versionage d'Electron

Si vous êtes un développeur Node expérimenté, vous êtes sûrement au courant de `semver` - et pourrez l'utiliser pour donner à vos systèmes de gestion de dépendences seulement des lignes directrices générales plutôt que des numéros de version fixes. En raison d'une forte dépendence avec Node et
Chromium, Electron est dans une position quelque peu difficile et ne suit pas
semver. Vous devez donc toujours faire référence à une version spécifique d'Electron.

Les numéros de version sont mis à jour selon les règle suivantes:

* Majeur: Pour les gros changements entrainant des ruptures dans l'API d'Electron - Si vous passez de la version `0.37.0`
  à `1.0.0`, vous devrez effectuer une migration de votre application.
* Mineur: Pour des changements majeurs de Chrome et des changements mineur de Node; ou  des changements important d'Electron - si vous mettez à jour de `1.0.0` vers `1.1.0`, le plus gros de votre application fonctionnera, seul de petits changements seront à effectuer.
* Patch: Pour de nouvelles fonctionalités et des résolutions de bugs - si vous passez de la version `1.0.0` à `1.0.1`, votre application continuera de s'exécuter telle quelle.

Si vous utilisez `electron` ou `electron-prebuilt`, nous vous recommandons de fixer le numéro de version (`1.1.0` instead of `^1.1.0`) pour être sûr que toutes les mises à jour d'Electron sont une opération manuelle faite par vous, le développeur.
