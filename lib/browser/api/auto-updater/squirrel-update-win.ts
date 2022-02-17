import * as fs from 'fs';
import * as path from 'path';
import { spawn, ChildProcessWithoutNullStreams } from 'child_process';

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
const spawnUpdate = function (args: string[], detached: boolean, callback: Function) {
  let error: Error, errorEmitted: boolean, stderr: string, stdout: string;

  try {
    // Ensure we don't spawn multiple squirrel processes
    // Process spawned, same args:        Attach events to alread running process
    // Process spawned, different args:   Return with error
    // No process spawned:                Spawn new process
    if (spawnedProcess && !isSameArgs(args)) {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(`AutoUpdater process with arguments ${args} is already running`);
    } else if (!spawnedProcess) {
      spawnedProcess = spawn(updateExe, args, {
        detached: detached,
        windowsHide: true
      });
      spawnedArgs = args || [];
    }
  } catch (error1) {
    error = error1 as Error;

    // Shouldn't happen, but still guard it.
    process.nextTick(function () {
      return callback(error);
    });
    return;
  }
  stdout = '';
  stderr = '';

  spawnedProcess.stdout.on('data', (data) => { stdout += data; });
  spawnedProcess.stderr.on('data', (data) => { stderr += data; });

  errorEmitted = false;
  spawnedProcess.on('error', (error) => {
    errorEmitted = true;
    callback(error);
  });

  return spawnedProcess.on('exit', function (code, signal) {
    spawnedProcess = undefined;
    spawnedArgs = [];

    // We may have already emitted an error.
    if (errorEmitted) {
      return;
    }

    // Process terminated with error.
    if (code !== 0) {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(`Command failed: ${signal != null ? signal : code}\n${stderr}`);
    }

    // Success.
    callback(null, stdout);
  });
};

// Start an instance of the installed app.
export function processStart () {
  return spawnUpdate(['--processStartAndWait', exeName], true, function () {});
}

// Download the releases specified by the URL and write new results to stdout.
export function checkForUpdate (updateURL: string, callback: (error: Error | null, update?: any) => void) {
  return spawnUpdate(['--checkForUpdate', updateURL], false, function (error: Error, stdout: string) {
    let ref, ref1, update;
    if (error != null) {
      return callback(error);
    }
    try {
      // Last line of output is the JSON details about the releases
      const json = stdout.trim().split('\n').pop();
      update = (ref = JSON.parse(json!)) != null ? (ref1 = ref.releasesToApply) != null ? typeof ref1.pop === 'function' ? ref1.pop() : undefined : undefined : undefined;
    } catch {
      // Disabled for backwards compatibility:
      // eslint-disable-next-line standard/no-callback-literal
      return callback(new Error(`Invalid result:\n${stdout}`));
    }
    return callback(null, update);
  });
}

// Update the application to the latest remote version specified by URL.
export function update (updateURL: string, callback: (error: Error) => void) {
  return spawnUpdate(['--update', updateURL], false, callback);
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
