import * as cp from 'node:child_process';
import * as fs from 'fs-extra';
import * as os from 'node:os';
import * as path from 'node:path';

export async function copyApp (targetDir: string): Promise<string> {
  // On macOS we can just copy the app bundle, easier too because of symlinks
  if (process.platform === 'darwin') {
    const appBundlePath = path.resolve(process.execPath, '../../..');
    const newPath = path.resolve(targetDir, 'Electron.app');
    cp.spawnSync('cp', ['-R', appBundlePath, path.dirname(newPath)]);
    return newPath;
  }

  // On windows and linux we should read the zip manifest files and then copy each of those files
  // one by one
  const baseDir = path.dirname(process.execPath);
  const zipManifestPath = path.resolve(__dirname, '..', '..', 'script', 'zip_manifests', `dist_zip.${process.platform === 'win32' ? 'win' : 'linux'}.${process.arch}.manifest`);
  const filesToCopy = (await fs.promises.readFile(zipManifestPath, 'utf-8')).split('\n');
  await Promise.all(
    filesToCopy.map(rel =>
      fs.promises.cp(path.resolve(baseDir, rel), path.resolve(targetDir, rel))
    )
  );

  return path.resolve(targetDir, path.basename(process.execPath));
}

export async function withTempDirectory (fn: (dir: string) => Promise<void>, autoCleanUp = true) {
  const dir = await fs.mkdtemp(path.resolve(os.tmpdir(), 'electron-update-spec-'));
  try {
    await fn(dir);
  } finally {
    if (autoCleanUp) {
      cp.spawnSync('rm', ['-r', dir]);
    }
  }
};
