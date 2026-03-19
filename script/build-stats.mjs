import { createHash } from 'node:crypto';
import * as fs from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';

import { getChromiumVersionFromDEPS } from './lib/utils.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ELECTRON_DIR = resolve(__dirname, '..');

function getCommonTags () {
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

  return tags;
}

async function uploadSeriesToDatadog (series) {
  await fetch('https://api.datadoghq.com/api/v2/series', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'DD-API-KEY': process.env.DD_API_KEY
    },
    body: JSON.stringify({ series })
  });
}

async function uploadCacheHitRateStats (hitRate, stats) {
  const timestamp = Math.round(new Date().getTime() / 1000);
  const tags = getCommonTags();

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

  await uploadSeriesToDatadog(series);
}

async function uploadObjectChangeStats (stats) {
  const timestamp = Math.round(new Date().getTime() / 1000);
  const tags = getCommonTags();

  if (stats['previous-chromium-version']) tags.push(`previous-chromium-version:${stats['previous-chromium-version']}`);
  if (stats['chromium-version']) tags.push(`chromium-version:${stats['chromium-version']}`);

  if (stats['previous-chromium-version'] && stats['chromium-version'] && stats['previous-chromium-version'] !== stats['chromium-version']) {
    tags.push('chromium-version-changed:true');
  }

  const series = [
    {
      metric: 'electron.build.object-change-rate',
      points: [{ timestamp, value: (stats['change-rate'] * 100).toFixed(2) }],
      type: 3, // GAUGE
      unit: 'percent',
      tags
    },
    {
      metric: 'electron.build.object-change-size',
      points: [{ timestamp, value: stats['change-size'] }],
      type: 1, // COUNT
      unit: 'byte',
      tags
    },
    {
      metric: 'electron.build.new-object-count',
      points: [{ timestamp, value: stats['new-object-count'] }],
      type: 1, // COUNT
      unit: 'count',
      tags
    }
  ];

  await uploadSeriesToDatadog(series);
}

async function main () {
  const { positionals: [filename], values } = parseArgs({
    allowPositionals: true,
    options: {
      'upload-stats': {
        type: 'boolean',
        default: false
      },
      'out-dir': {
        type: 'string'
      },
      'input-object-checksums': {
        type: 'string'
      },
      'output-object-checksums': {
        type: 'string'
      }
    }
  });

  const {
    'upload-stats': uploadStats,
    'out-dir': outDir,
    'input-object-checksums': inputObjectChecksums,
    'output-object-checksums': outputObjectChecksums
  } = values;

  if (!filename) {
    throw new Error('filename is required (should be a siso.INFO file)');
  }

  if ((inputObjectChecksums || outputObjectChecksums) && !outDir) {
    throw new Error('--out-dir is required when using --input-object-checksums or --output-object-checksums');
  } else if (outDir && (!inputObjectChecksums && !outputObjectChecksums)) {
    throw new Error('--out-dir only makes sense with --input-object-checksums or --output-object-checksums');
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

  const messagePrefix = process.env.GITHUB_ACTIONS ? '::notice title=Build Stats::' : '';

  console.log(`${messagePrefix}Effective cache hit rate: ${(hitRate * 100).toFixed(2)}%`);

  const objectChangeStats = {};

  if (inputObjectChecksums || outputObjectChecksums) {
    const depsContent = await fs.readFile(resolve(ELECTRON_DIR, 'DEPS'), 'utf8');
    const currentVersion = getChromiumVersionFromDEPS(depsContent);

    // Calculate the SHA256 for each object file under `outDir`
    const objectFiles = await fs.readdir(outDir, { encoding: 'utf8', recursive: true });
    const checksums = {};
    for (const file of objectFiles.filter(f => f.endsWith('.o'))) {
      const content = await fs.readFile(resolve(outDir, file));
      checksums[file] = createHash('sha256').update(content).digest('hex');
    }

    if (outputObjectChecksums) {
      const outputData = {
        chromiumVersion: currentVersion,
        checksums
      };

      await fs.writeFile(outputObjectChecksums, JSON.stringify(outputData, null, 2));
    }

    if (inputObjectChecksums) {
      const inputData = JSON.parse(await fs.readFile(inputObjectChecksums, 'utf8'));
      const inputFiles = Object.keys(inputData.checksums);
      let changedCount = 0;
      let newObjectCount = 0;
      let changedSize = 0;

      // Count changed files (only those present in both input and current)
      for (const file of inputFiles) {
        if (!(file in checksums)) continue; // Skip deleted files
        if (inputData.checksums[file] !== checksums[file]) {
          changedCount++;
          const stat = await fs.stat(resolve(outDir, file));
          changedSize += stat.size;
        }
      }

      // Count new files (in current but not in input)
      for (const file of Object.keys(checksums)) {
        if (!(file in inputData.checksums)) {
          newObjectCount++;
          const stat = await fs.stat(resolve(outDir, file));
          changedSize += stat.size;
        }
      }

      const changeRate = inputFiles.length > 0 ? changedCount / inputFiles.length : 0;
      console.log(`${messagePrefix}Object change rate: ${(changeRate * 100).toFixed(2)}%`);
      console.log(`${messagePrefix}New object count: ${newObjectCount}`);
      console.log(`${messagePrefix}Cumulative changed object sizes: ${changedSize.toLocaleString()} bytes`);

      objectChangeStats['change-rate'] = changeRate;
      objectChangeStats['change-size'] = changedSize;
      objectChangeStats['new-object-count'] = newObjectCount;
      objectChangeStats['previous-chromium-version'] = inputData.chromiumVersion;
      objectChangeStats['chromium-version'] = currentVersion;
    }
  }

  if (uploadStats) {
    if (!process.env.DD_API_KEY) {
      throw new Error('DD_API_KEY is not set');
    }

    await uploadCacheHitRateStats(hitRate, stats);

    if (Object.keys(objectChangeStats).length > 0) {
      await uploadObjectChangeStats(objectChangeStats);
    }
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
