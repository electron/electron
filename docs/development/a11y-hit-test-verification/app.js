// Application de test Electron pour verifier le hit-test d'accessibilite.
//
// But : reproduire le bug electron/electron#42945, c'est-a-dire le cas ou la
// fenetre expose une « pane » vide a la place du contenu web sur le chemin
// MSAA (AccessibleObjectFromPoint), celui qu'utilise le mode herite de NVDA.
//
// Lancement : electron.exe <dossier de ce fichier>
// Le controleur (uia_check.ps1) lit ensuite le fichier de mesures ecrit dans
// le repertoire temporaire.
//
// La page positionne ses elements de façon absolue : chaque point de controle
// tombe donc exactement sur un element de texte connu.

const { app, BrowserWindow, View, screen } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');

// Active l'accessibilite dans les deux processus, comme le demande le
// rapport du bug (#42945) : l'arbre d'accessibilite doit etre publie.
app.commandLine.appendSwitch('force-renderer-accessibility');

const sortie = path.join(os.tmpdir(), 'electron-a11y-hit-test.json');

const PAGE = `<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Page de test accessibilite</title>
<style>
  body { margin: 0; font-family: sans-serif; }
  .bloc { position: absolute; border: 1px solid #333; padding: 4px; }
</style></head>
<body>
  <div class="bloc" style="left:60px;  top:60px;  width:320px; height:40px;">TITRE DE L ARTICLE</div>
  <div class="bloc" style="left:60px;  top:160px; width:320px; height:40px;">PARAGRAPHE DE TEST</div>
  <div class="bloc" style="left:60px;  top:260px; width:320px; height:40px;">PREMIER ELEMENT DE LISTE</div>
  <div class="bloc" style="left:60px;  top:360px; width:320px; height:40px;">BOUTON FACTICE</div>
  <div class="bloc" style="left:450px; top:350px; width:300px; height:40px;">ELEMENT AU CENTRE</div>
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

  win.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(PAGE));

  // Une vue enfant de 40x40 dans le coin superieur gauche de la contentView.
  // Elle rend la contentView « non vide » : c'est le second cas du tableau de
  // verification de la PR d'origine, ou la contentView a des enfants.
  const enfant = new View();
  enfant.setBounds({ x: 10, y: 10, width: 40, height: 40 });
  enfant.setBackgroundColor('#ff0000');
  win.contentView.addChildView(enfant);

  // Le fichier de mesures est ecrit des que la fenetre existe : le
  // chargement de la page n'est pas necessaire pour interroger le hit-test.
  let ecrit = false;
  const ecrireSiNecesaire = () => {
    if (ecrit) return;
    ecrit = true;
    // Bornes de la zone de contenu, en pixels logiques (DIP) et relatives a
    // l'ecran, ainsi que le facteur d'echelle de l'ecran : le controleur
    // convertit en pixels physiques, ceux que renvoie l'API d'accessibilite.
    const bornes = win.getContentBounds();
    const echelle = screen.getDisplayMatching(bornes).scaleFactor;
    ecrire({
      pid: process.pid,
      titre: win.getTitle(),
      echelle: echelle,
      bornes: {
        x: bornes.x,
        y: bornes.y,
        largeur: bornes.width,
        hauteur: bornes.height
      },
      points: [
        { nom: 'centre', x: 450, y: 350 },
        { nom: 'titre-article', x: 60, y: 60 },
        { nom: 'paragraphe', x: 60, y: 160 },
        { nom: 'liste', x: 60, y: 260 },
        { nom: 'bouton', x: 60, y: 360 },
        { nom: 'enfant-coin', x: 30, y: 30 }
      ]
    });
    console.log('FENETRE_PRET');
  };

  win.once('ready-to-show', ecrireSiNecesaire);
  setTimeout(ecrireSiNecesaire, 2500);

  win.on('closed', () => app.quit());
});
