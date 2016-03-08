'use strict';

const fs = require('fs');
const path = require('path');

function updateFiles(electronDir, libchromiumcontent) {
  let files = fs.readdirSync(electronDir);

  for (var i=0; i < files.length; i++) {
    let current = path.join(electronDir, files[i]);
    let chromePath = path.join(libchromiumcontent, files[i]);
    let stat = fs.statSync(current);

    if (stat.isDirectory()) {
      if (!fs.existsSync(chromePath)) {
        console.error("Warning! Tried to recurse into path but it doesn't exist in libchromiumcontent! " + chromePath);
        fs.renameSync(current, current + "__deleteme");
        continue;
      }

      updateFiles(current, chromePath);
      continue;
    }

    if (!fs.existsSync(chromePath)) {
      console.error("Warning! Tried to read file but it doesn't exist in libchromiumcontent! " + chromePath);
      fs.renameSync(current, current + "__deleteme");
      continue;
    }

    //console.log("     Updating " + chromePath + " => " + current);
    let buf = fs.readFileSync(chromePath);
    fs.writeFileSync(current);
  }
}

const toUpdate = path.resolve(__dirname, '..', 'chromium_src');
const libchromiumcontent = process.argv[2];

updateFiles(toUpdate, libchromiumcontent);
