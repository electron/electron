import * as childProcess from 'child_process';
import * as fs from 'fs';
import * as klaw from 'klaw';
import * as minimist from 'minimist';
import * as os from 'os';
import * as path from 'path';
import * as streamChain from 'stream-chain';
import * as streamJson from 'stream-json';
import { ignore as streamJsonIgnore } from 'stream-json/filters/Ignore';
import { streamArray as streamJsonStreamArray } from 'stream-json/streamers/StreamArray';

import { chunkFilenames } from './lib/utils';

const SOURCE_ROOT = path.normalize(path.dirname(__dirname));
const LLVM_BIN = path.resolve(
  SOURCE_ROOT,
  '..',
  'third_party',
  'llvm-build',
  'Release+Asserts',
  'bin'
);
const PLATFORM = os.platform();

type SpawnAsyncResult = {
  stdout: string;
  stderr: string;
  status: number | null;
};

class ErrorWithExitCode extends Error {
  exitCode: number;

  constructor (message: string, exitCode: number) {
    super(message);
    this.exitCode = exitCode;
  }
}

async function spawnAsync (
  command: string,
  args: string[],
  options?: childProcess.SpawnOptionsWithoutStdio | undefined
): Promise<SpawnAsyncResult> {
  return new Promise((resolve, reject) => {
    try {
      const stdio = { stdout: '', stderr: '' };
      const spawned = childProcess.spawn(command, args, options || {});

      spawned.stdout.on('data', (data) => {
        stdio.stdout += data;
      });

      spawned.stderr.on('data', (data) => {
        stdio.stderr += data;
      });

      spawned.on('exit', (code) => resolve({ ...stdio, status: code }));
      spawned.on('error', (err) => reject(err));
    } catch (err) {
      reject(err);
    }
  });
}

function getDepotToolsEnv (): NodeJS.ProcessEnv {
  let depotToolsEnv;

  const findDepotToolsOnPath = () => {
    const result = childProcess.spawnSync(
      PLATFORM === 'win32' ? 'where' : 'which',
      ['gclient']
    );

    if (result.status === 0) {
      return process.env;
    }
  };

  const checkForBuildTools = () => {
    const result = childProcess.spawnSync(
      'electron-build-tools',
      ['show', 'env', '--json'],
      { shell: true }
    );

    if (result.status === 0) {
      return {
        ...process.env,
        ...JSON.parse(result.stdout.toString().trim())
      };
    }
  };

  try {
    depotToolsEnv = findDepotToolsOnPath();
    if (!depotToolsEnv) depotToolsEnv = checkForBuildTools();
  } catch {}

  if (!depotToolsEnv) {
    throw new Error("Couldn't find depot_tools, ensure it's on your PATH");
  }

  if (!('CHROMIUM_BUILDTOOLS_PATH' in depotToolsEnv)) {
    throw new Error(
      'CHROMIUM_BUILDTOOLS_PATH environment variable must be set'
    );
  }

  return depotToolsEnv;
}

async function runClangTidy (
  outDir: string,
  filenames: string[],
  checks: string = '',
  jobs: number = 1
): Promise<boolean> {
  const cmd = path.resolve(LLVM_BIN, 'clang-tidy');
  const args = [`-p=${outDir}`, '--use-color'];

  if (checks) args.push(`--checks=${checks}`);

  // Remove any files that aren't in the compilation database to prevent
  // errors from cluttering up the output. Since the compilation DB is hundreds
  // of megabytes, this is done with streaming to not hold it all in memory.
  const filterCompilationDatabase = (): Promise<string[]> => {
    const compiledFilenames: string[] = [];

    return new Promise((resolve) => {
      const pipeline = streamChain.chain([
        fs.createReadStream(path.resolve(outDir, 'compile_commands.json')),
        streamJson.parser(),
        streamJsonIgnore({ filter: /\bcommand\b/i }),
        streamJsonStreamArray(),
        ({ value: { file, directory } }) => {
          const filename = path.resolve(directory, file);
          return filenames.includes(filename) ? filename : null;
        }
      ]);

      pipeline.on('data', (data) => compiledFilenames.push(data));
      pipeline.on('end', () => resolve(compiledFilenames));
    });
  };

  // clang-tidy can figure out the file from a short relative filename, so
  // to get the most bang for the buck on the command line, let's trim the
  // filenames to the minimum so that we can fit more per invocation
  filenames = (await filterCompilationDatabase()).map((filename) =>
    path.relative(SOURCE_ROOT, filename)
  );

  if (filenames.length === 0) {
    throw new Error('No filenames to run');
  }

  const commandLength =
    cmd.length + args.reduce((length, arg) => length + arg.length, 0);

  const results: boolean[] = [];
  const asyncWorkers = [];
  const chunkedFilenames: string[][] = [];

  const filesPerWorker = Math.ceil(filenames.length / jobs);

  for (let i = 0; i < jobs; i++) {
    chunkedFilenames.push(
      ...chunkFilenames(filenames.splice(0, filesPerWorker), commandLength)
    );
  }

  const worker = async () => {
    let filenames = chunkedFilenames.shift();

    while (filenames?.length) {
      results.push(
        await spawnAsync(cmd, [...args, ...filenames], {}).then((result) => {
          console.log(result.stdout);

          if (result.status !== 0) {
            console.error(result.stderr);
          }

          // On a clean run there's nothing on stdout. A run with warnings-only
          // will have a status code of zero, but there's output on stdout
          return result.status === 0 && result.stdout.length === 0;
        })
      );

      filenames = chunkedFilenames.shift();
    }
  };

  for (let i = 0; i < jobs; i++) {
    asyncWorkers.push(worker());
  }

  try {
    await Promise.all(asyncWorkers);
    return results.every((x) => x);
  } catch {
    return false;
  }
}

async function findMatchingFiles (
  top: string,
  test: (filename: string) => boolean
): Promise<string[]> {
  return new Promise((resolve) => {
    const matches = [] as string[];
    klaw(top, {
      filter: (f) => path.basename(f) !== '.bin'
    })
      .on('end', () => resolve(matches))
      .on('data', (item) => {
        if (test(item.path)) {
          matches.push(item.path);
        }
      });
  });
}

function parseCommandLine () {
  const showUsage = (arg?: string) : boolean => {
    if (!arg || arg.startsWith('-')) {
      console.log(
        'Usage: script/run-clang-tidy.ts [-h|--help] [--jobs|-j] ' +
          '[--checks] --out-dir OUTDIR [file1 file2]'
      );
      process.exit(0);
    }

    return true;
  };

  const opts = minimist(process.argv.slice(2), {
    boolean: ['help'],
    string: ['checks', 'out-dir'],
    default: { jobs: 1 },
    alias: { help: 'h', jobs: 'j' },
    stopEarly: true,
    unknown: showUsage
  });

  if (opts.help) showUsage();

  if (!opts['out-dir']) {
    console.log('--out-dir is a required argument');
    process.exit(0);
  }

  return opts;
}

async function main (): Promise<boolean> {
  const opts = parseCommandLine();
  const outDir = path.resolve(opts['out-dir']);

  if (!fs.existsSync(outDir)) {
    throw new Error("Output directory doesn't exist");
  } else {
    // Make sure the compile_commands.json file is up-to-date
    const env = getDepotToolsEnv();

    const result = childProcess.spawnSync(
      'gn',
      ['gen', '.', '--export-compile-commands'],
      { cwd: outDir, env, shell: true }
    );

    if (result.status !== 0) {
      if (result.error) {
        console.error(result.error.message);
      } else {
        console.error(result.stderr.toString());
      }

      throw new ErrorWithExitCode(
        'Failed to automatically generate compile_commands.json for ' +
          'output directory',
        2
      );
    }
  }

  const filenames = [];

  if (opts._.length > 0) {
    filenames.push(...opts._.map((filename) => path.resolve(filename)));
  } else {
    filenames.push(
      ...(await findMatchingFiles(
        path.resolve(SOURCE_ROOT, 'shell'),
        (filename: string) => /.*\.(?:cc|h|mm)$/.test(filename)
      ))
    );
  }

  return runClangTidy(outDir, filenames, opts.checks, opts.jobs);
}

if (require.main === module) {
  main()
    .then((success) => {
      process.exit(success ? 0 : 1);
    })
    .catch((err: ErrorWithExitCode) => {
      console.error(`ERROR: ${err.message}`);
      process.exit(err.exitCode || 1);
    });
}
