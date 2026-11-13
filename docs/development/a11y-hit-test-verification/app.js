// Test Electron application for the accessibility hit-test.
//
// Purpose: reproduce electron/electron#42945, that is the case where the
// window exposes an empty "pane" instead of the web content on the MSAA path
// (AccessibleObjectFromPoint), the path NVDA uses in its legacy mode.
//
// Usage: electron.exe <folder of this file> [--avec-enfant]
// The checker (uia_check.ps1) then reads the measurements file written in the
// temporary directory.
//
// Without the --avec-enfant option the contentView stays empty: that is the
// real case of a BrowserWindow that adds no view. With the option, a 40 x 40
// view is added in the corner of the contentView: the contentView is not
// empty any more, so it has to stay in the accessibility tree.
//
// The page positions its elements absolutely: every control point therefore
// lands exactly on a known text element.

const { app, BrowserWindow, View, screen } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');

// Enables accessibility in both processes, as the bug report asks for
// (#42945): the accessibility tree has to be published.
app.commandLine.appendSwitch('force-renderer-accessibility');

const sortie = path.join(os.tmpdir(), 'electron-a11y-hit-test.json');

// Command line option: adds a child view to the contentView.
const avecEnfant = process.argv.includes('--avec-enfant');

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

  // A 40 x 40 child view in the top left corner of the contentView. It makes
  // the contentView "not empty": that is the second case of the verification
  // table of the original PR, where the contentView has children.
  if (avecEnfant) {
    const enfant = new View();
    enfant.setBounds({ x: 10, y: 10, width: 40, height: 40 });
    enfant.setBackgroundColor('#ff0000');
    win.contentView.addChildView(enfant);
  }

  // The measurements file is written as soon as the window exists: loading
  // the page is not needed to query the hit-test.
  let ecrit = false;
  const ecrireSiNecesaire = () => {
    if (ecrit) return;
    ecrit = true;
    // Bounds of the content area, in logical pixels (DIP) and relative to the
    // screen, plus the screen scale factor: the checker converts them to
    // physical pixels, the ones the accessibility API returns.
    const bornes = win.getContentBounds();
    const echelle = screen.getDisplayMatching(bornes).scaleFactor;
    ecrire({
      pid: process.pid,
      titre: win.getTitle(),
      avec_enfant: avecEnfant,
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
        { nom: 'coin', x: 30, y: 30 }
      ]
    });
    console.log('FENETRE_PRET');
  };

  win.once('ready-to-show', ecrireSiNecesaire);
  setTimeout(ecrireSiNecesaire, 2500);

  win.on('closed', () => app.quit());
});