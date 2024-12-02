import { spawn, ChildProcessWithoutNullStreams } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';

// i.e. my-app/app-0.1.13/
const appFolder = path.dirname(process.execPath);

// i.e. my-app/Update.exe
const updateExe = path.resolve(appFolder, '..', 'Update.exe');
const exeName = path.basename(process.execPath);
let spawnedArgs: string[] = [];
let spawnedProcess: ChildProcessWithoutNullStreams | undefined;

const isSameArgs = (args: string[]) => args.length === spawnedArgs.length && args.every((e, i) => e === spawnedArgs[i]);

// Spawn a command and invoke the callback when it completes with an error
// and the output from standard out.
const spawnUpdate = async function (args: string[], options: { detached: boolean }): Promise<string> {
  return new Promise((resolve, reject) => {
    // Ensure we don't spawn multiple squirrel processes
    // Process spawned, same args:        Attach events to already running process
    // Process spawned, different args:   Return with error
    // No process spawned:                Spawn new process
    if (spawnedProcess && !isSameArgs(args)) {
      throw new Error(`AutoUpdater process with arguments ${args} is already running`);
    } else if (!spawnedProcess) {
      spawnedProcess = spawn(updateExe, args, {
        detached: options.detached,
        windowsHide: true
      });
      spawnedArgs = args || [];
    }

    let stdout = '';
    let stderr = '';

    spawnedProcess.stdout.on('data', (data) => { stdout += data; });
    spawnedProcess.stderr.on('data', (data) => { stderr += data; });

    spawnedProcess.on('error', (error) => {
      spawnedProcess = undefined;
      spawnedArgs = [];
      reject(error);
    });

    spawnedProcess.on('exit', function (code, signal) {
      spawnedProcess = undefined;
      spawnedArgs = [];

      if (code !== 0) {
        // Process terminated with error.
        reject(new Error(`Command failed: ${signal ?? code}\n${stderr}`));
      } else {
        // Success.
        resolve(stdout);
      }
    });
  });
};

// Start an instance of the installed app.
export function processStart () {
  spawnUpdate(['--processStartAndWait', exeName], { detached: true });
}

// Download the releases specified by the URL and write new results to stdout.
export async function checkForUpdate (updateURL: string): Promise<any> {
  const stdout = await spawnUpdate(['--checkForUpdate', updateURL], { detached: false });
  try {
    // Last line of output is the JSON details about the releases
    const json = stdout.trim().split('\n').pop();
    return JSON.parse(json!)?.releasesToApply?.pop?.();
  } catch {
    throw new Error(`Invalid result:\n${stdout}`);
  }
}

// Update the application to the latest remote version specified by URL.
export async function update (updateURL: string): Promise<void> {
  await spawnUpdate(['--update', updateURL], { detached: false });
}

// Is the Update.exe installed with the current application?
export function supported () {
  try {
    fs.accessSync(updateExe, fs.constants.R_OK);
    return true;
  } catch {
    return false;
  }
}
