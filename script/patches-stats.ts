import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { parseArgs } from 'node:util';

async function main (): Promise<void> {
  const { values: { 'upload-stats': uploadStats } } = parseArgs({
    options: {
      'upload-stats': {
        type: 'boolean',
        default: false
      }
    }
  });

  const config = JSON.parse(await fs.readFile(path.join(__dirname, '..', 'patches', 'config.json'), 'utf-8'));

  let patchCount = 0;
  let patchesLineCount = 0;

  const root = path.join(__dirname, '..', '..', '..');

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
            metric: 'patches.count',
            points: [
              {
                timestamp,
                value: patchCount
              }
            ],
            type: 1 // COUNT
          },
          {
            metric: 'patches.lineCount',
            points: [
              {
                timestamp,
                value: patchesLineCount
              }
            ],
            type: 1 // COUNT
          }
        ]
      })
    });
  }
}

if (require.main === module) {
  main()
    .then(() => {
      process.exit(0);
    })
    .catch((err: unknown) => {
      console.error(`ERROR: ${(err as Error).message}`);
      process.exit(1);
    });
}
