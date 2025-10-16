import * as fs from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

async function main () {
  const { positionals: [filename], values: { 'upload-stats': uploadStats } } = parseArgs({
    allowPositionals: true,
    options: {
      'upload-stats': {
        type: 'boolean',
        default: false
      }
    }
  });

  if (!filename) {
    throw new Error('filename is required (should be a siso.INFO file)');
  }

  const log = await fs.readFile(filename, 'utf-8');

  // We expect to find a line which looks like stats=build.Stats{..., CacheHit:39008, Local:4778, Remote:0, LocalFallback:0, ...}
  const match = log.match(/stats=build\.Stats{(.*)}/);

  if (!match) {
    throw new Error('could not find stats=build.Stats in log');
  }

  const stats = Object.fromEntries(match[1].split(',').map(part => {
    const [key, value] = part.trim().split(':');
    return [key, parseInt(value)];
  }));
  const hitRate = stats.CacheHit / (stats.Remote + stats.CacheHit + stats.LocalFallback);

  console.log(`Effective cache hit rate: ${(hitRate * 100).toFixed(2)}%`);

  if (uploadStats) {
    if (!process.env.DD_API_KEY) {
      throw new Error('DD_API_KEY is not set');
    }

    const timestamp = Math.round(new Date().getTime() / 1000);

    const tags = [];

    if (process.env.TARGET_ARCH) tags.push(`target-arch:${process.env.TARGET_ARCH}`);
    if (process.env.TARGET_PLATFORM) tags.push(`target-platform:${process.env.TARGET_PLATFORM}`);
    if (process.env.GITHUB_HEAD_REF) {
      // Will be set in pull requests
      tags.push(`branch:${process.env.GITHUB_HEAD_REF}`);
    } else if (process.env.GITHUB_REF_NAME) {
      // Will be set for release branches
      tags.push(`branch:${process.env.GITHUB_REF_NAME}`);
    }

    const series = [
      {
        metric: 'electron.build.effective-cache-hit-rate',
        points: [{ timestamp, value: (hitRate * 100).toFixed(2) }],
        type: 3, // GAUGE
        unit: 'percent',
        tags
      }
    ];

    // Add all raw stats as individual metrics
    for (const [key, value] of Object.entries(stats)) {
      series.push({
        metric: `electron.build.stats.${key.toLowerCase()}`,
        points: [{ timestamp, value }],
        type: 1, // COUNT
        tags
      });
    }

    await fetch('https://api.datadoghq.com/api/v2/series', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'DD-API-KEY': process.env.DD_API_KEY
      },
      body: JSON.stringify({ series })
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
