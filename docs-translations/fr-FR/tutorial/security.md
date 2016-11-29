# Securité, Application Natives, et Votre Responsabilité

En tant que développeurs Web, nous avons l'habitude de bénéficier d'une sécurité élevée au niveau du navigateur - les
risques associés au code que nous écrivons sont relativement faibles. Nos sites internet ont des droit limités au sein 
d'une sandbox, et nous faisons confiance à nos utilisateurs pour utiliser des navigateurs créés par de grosses équipes d'ingénieurs 
capables de réagir rapidement lorsqu'une faille de sécurité est découverte.

Quand on travaille avec Electron, il est important de comprendre qu'Electron n'est pas un navigateur web.
Il vous permet de construire des applications de bureau riches de fonctionnalités au travers de technologies web familaires,
mais votre code a beaucoup plus de possibilités. Javascript peut accéder au système de fichier, au shell, et plus. 
Cela vous permet de construire des applications natives de haute qualité, mais les problèmes de sécurité sont inhérents à toutes ces possibilités.

Avec ça en tête, soyez conscient qu'afficher du contenu arbitraire depuis des sources extérieures pose un gros risque au niveau de la sécurité qu'Electron ne peut gérer.
En fait, les applications utilisant Electron les plus populaires (Atom, Slack, Visual Studio Code, ...etc) affichent principalement du contenu local (ou de confiance, il s'agit alors de contenu distant sécurisé sans intégration avec Node) - si votre application exécute du code depuis une source extérieur, il est de votre responsabilité de vous assurer que ce code n'est pas malveillant.


## Problèmes de sécurités liés à Chromium et mises à jour

Tandis qu'Electron essai de supporter les nouvelles versions de Chromium dès que possible,
les developpeurs doivent garder à l'esprit que le fait de mettre à jour l'application est une tâche laborieuse durant laquelle plusieurs douzaines, voir plusieurs centaines de fichiers doivent être modifiés à la main.
Selon les ressources et les contributions actuelles, Electron ne fonctionnera pas toujours avec la dernière version de Chromium, un délai de quelques jours voir quelques semaines est à prévoir.

Nous pensons que notre système actuel de mises à jour du composant Chromium correspond à
équilibre approprié entre les ressources dont nous disposons et les besoins de la
majorité des applications construites autour du framework.
Les Pull requests et les contributions supportant cet effort sont toujours les bienvenues.

## Ignorer les conseils précédents

A security issue exists whenever you receive code from a remote destination and
execute it locally. Prenons comme exemple l'affichage d'un site web distant affiché à l'intérieur d'une fenêtre de navigateur.
Si un attaquant parvient d'une quelconque façon de changer son contenu
(soit en attaquant la source directement, ou bien en se placant entre votre application et sa destination actuelle), ils seront capables d'executer du code natif sur la machine de l'utilisateur.

> :warning: En aucun cas vous ne devez charger puis exécuter du code distant avec Node. A la place, utilisez seulement des fichiers locaux (regroupés avec votre application) pour exécuter du code de Node. Pour afficher du contenu distant, utilisez le tag
`webview` et assurez vous de désactiver `nodeIntegration`.

#### Checklist

Il ne s'agit pas d'une liste exhaustive, mais au moins, pour palier aux problèmes de sécurités vous devez essayer de:

* Afficher seulement du contenu (https) sécurisé
* Désactiver l'intégration de Node dans tout ce qui gère le rendu avec du contenu distant
  (using `webPreferences`)
* Ne pas désactiver `webSecurity`. Disabling it will disable the same-origin policy.
* Définir une [`Content-Security-Policy`](http://www.html5rocks.com/en/tutorials/security/content-security-policy/)
, et utiliser des règles strictes (i.e. `script-src 'self'`)
* [Surcharger et désactiver `eval`](https://github.com/nylas/N1/blob/0abc5d5defcdb057120d726b271933425b75b415/static/index.js#L6-L8)
, qui permet à des chaines de caractères d'être exécutées comme du code.
* Ne pas assigner `allowDisplayingInsecureContent` à true.
* Ne pas assigner `allowRunningInsecureContent` à true.
* Ne pas activer `experimentalFeatures` ou `experimentalCanvasFeatures` à moins d'être sûr ce que vous faites.
* Ne pas utiliser `blinkFeatures` à moins d'être sûr ce que vous faites.
* WebViews: Assigner `nodeintegration` à false
* WebViews: Ne pas utiliser `disablewebsecurity`
* WebViews: Ne pas utiliser `allowpopups`
* WebViews: Ne pas utiliser `insertCSS` ou `executeJavaScript` avec du CSS/JS distant.

Encore une fois, cette liste permet de diminuer les risques de problème de sécurité, mais en aucun cas elle ne l'enlève complètement. Si votre objectif est d'afficher un site internet, choisir un navigateur sera une option plus sûre.

## Buffer Global

La classe [Buffer](https://nodejs.org/api/buffer.html) de Node est actuellement disponible
en tant que global même quand `nodeIntegration` est à `false`. Vous pouvez le supprimer en faisant la manipulation suivante dans votre script `preload`:

```js
delete global.Buffer
```

Le supprimer peut casser les modules Node utilisés dans votre script preload script et votre application depuis que plusieurs librairies s'attendent à ce qu'il soit en tant que global plutôt que de le demander de manière explicite via:

```js
const {Buffer} = require('buffer')
```

Le `Buffer` global risque d'être supprimé dans de futures versions d'Electron.

