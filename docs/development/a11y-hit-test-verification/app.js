// Application de test Electron pour verifier le hit-test d'accessibilite.
//
// But : reproduire le bug electron/electron#42945, c'est-a-dire le cas ou la
// fenetre expose une « pane » vide a la place du contenu web, ce qui casse le
// suivi de souris de NVDA.
//
// Lancement : electron.exe <dossier de ce fichier>
// La position de la fenetre et les points a tester sont ecrits dans
// window.json, que le controleur UIA (uia_check.ps1) relit ensuite.

const { app, BrowserWindow, View } = require('electron');
const fs = require('fs');
const path = require('path');

// Active l'accessibilite dans les deux processus, comme le demande le
// rapport du bug (#42945) : l'arbre d'accessibilite doit etre publie.
app.commandLine.appendSwitch('force-renderer-accessibility');

const sortie = path.join(__dirname, 'window.json');

const PAGE = `<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Page de test accessibilite</title>
<style>
  body { font-family: sans-serif; padding: 20px; }
  .cadre { border: 1px solid #333; padding: 10px; margin-bottom: 10px; }
</style></head>
<body>
  <h1>Titre de l'article de test</h1>
  <p class="cadre">Paragraphe de test accessible au suivi de souris</p>
  <ul>
    <li>Premier element de liste</li>
    <li>Deuxieme element de liste</li>
    <li>Troisieme element de liste</li>
  </ul>
  <div role="button" tabindex="0">Bouton factice</div>
</body>
</html>`;

function ecrire(objet) {
  fs.writeFileSync(sortie, JSON.stringify(objet, null, 2), 'utf8');
}

app.whenReady().then(() => {
  app.setAccessibilitySupportEnabled(true);

  const win = new BrowserWindow({
    width: 900,
    height: 700,
    show: true,
    title: 'Test hit-test accessibilite'
  });

  win.setAccessibilitySupportEnabled(true);
  win.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(PAGE));

  // Cas 2 du tableau de verification de la PR : une vue enfant de 40x40
  // dans le coin superieur gauche de la contentView. Elle rend la
  // contentView « non vide » et permet de verifier que le correctif
  // fonctionne aussi quand la contentView a des enfants.
  const enfant = new View();
  enfant.setBounds({ x: 10, y: 10, width: 40, height: 40 });
  enfant.setBackgroundColor('#ff0000');
  win.contentView.addChildView(enfant);

  win.webContents.once('did-finish-load', () => {
    setTimeout(() => {
      const bornes = win.getBounds();
      ecrire({
        pid: process.pid,
        titre: win.getTitle(),
        bornes: {
          x: bornes.x,
          y: bornes.y,
          largeur: bornes.width,
          hauteur: bornes.height
        },
        points: [
          { nom: 'centre', x: Math.round(bornes.width / 2), y: Math.round(bornes.height / 2) },
          { nom: 'titre-article', x: 60, y: 120 },
          { nom: 'paragraphe', x: 60, y: 200 },
          { nom: 'liste', x: 60, y: 300 },
          { nom: 'enfant-coin', x: 30, y: 30 }
        ]
      });
      console.log('FENETRE_PRET');
    }, 1500);
  });

  win.on('closed', () => app.quit());
});
