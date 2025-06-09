import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

async function main () {
  const { values: { 'upload-stats': uploadStats } } = parseArgs({
    options: {
      'upload-stats': {
        type: 'boolean',
        default: false
      }
    }
  });

  const config = JSON.parse(await fs.readFile(path.join(import.meta.dirname, '..', 'patches', 'config.json'), 'utf-8'));

  let patchCount = 0;
  let patchesLineCount = 0;

  const root = path.join(import.meta.dirname, '..', '..', '..');

  for (const target of config) {
    const patchDirPath = path.join(root, target.patch_dir);
    const patches = await fs.readFile(path.join(patchDirPath, '.patches'), 'utf-8');

    for (const patch of patches.trim().split('\n')) {
      patchCount++;
      const contents = await fs.readFile(path.join(patchDirPath, patch), 'utf-8');
      patchesLineCount += Array.from(contents.matchAll(/\n/g)).length;
    }
  }

  console.log(`Total patches: ${patchCount}`);
  console.log(`Total lines in patches: ${patchesLineCount}`);

  if (uploadStats) {
    if (!process.env.DD_API_KEY) {
      throw new Error('DD_API_KEY is not set');
    }

    const timestamp = Math.round(new Date().getTime() / 1000);

    await fetch('https://api.datadoghq.com/api/v2/series', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'DD-API-KEY': process.env.DD_API_KEY
      },
      body: JSON.stringify({
        series: [
          {
            metric: 'electron.patches.count',
            points: [
              {
                timestamp,
                value: patchCount
              }
            ],
            type: 3 // GAUGE
          },
          {
            metric: 'electron.patches.lineCount',
            points: [
              {
                timestamp,
                value: patchesLineCount
              }
            ],
            type: 3 // GAUGE
          }
        ]
      })
    });
  }
}

if ((await fs.realpath(process.argv[1])) === fileURLToPath(import.meta.url)) {
  main()
    .then(() => {
      process.exit(0);
    })
    .catch((err) => {
      console.error(`ERROR: ${err.message}`);
      process.exit(1);
    });
}
