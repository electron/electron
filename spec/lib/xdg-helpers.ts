import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

const fixturesPath = path.resolve(__dirname, '..', 'fixtures');
const xdgMockFixturePath = path.join(fixturesPath, 'api', 'xdg-mock');
const protocolLookupFixturePath = path.join(fixturesPath, 'api', 'protocol-name');

type ProtocolNameLookupResult = {
  name: string;
};

export type ProtocolInfoLookupResult = ProtocolNameLookupResult & {
  path: string;
  hasIcon: boolean;
};

export function makeXdgMockDirectories(prefix: string) {
  const xdgDir = fs.mkdtempSync(path.join(os.tmpdir(), prefix));
  fs.cpSync(xdgMockFixturePath, xdgDir, { recursive: true });

  const xdgDataHome = path.join(xdgDir, 'data');
  const xdgConfigHome = path.join(xdgDir, 'config');
  const xdgBinDir = path.join(xdgDir, 'bin');

  fs.chmodSync(path.join(xdgBinDir, 'xdg-mime'), 0o755);
  fs.chmodSync(path.join(xdgBinDir, 'xdg-settings'), 0o755);

  return { xdgDir, xdgDataHome, xdgConfigHome, xdgBinDir };
}

export function writeProtocolAssociation(
  xdgDataHome: string,
  xdgConfigHome: string,
  desktopFileId: string,
  displayName: string,
  execCommand: string,
  mimeType: string,
  iconPath?: string
) {
  const appsDir = path.join(xdgDataHome, 'applications');
  fs.mkdirSync(appsDir, { recursive: true });
  fs.mkdirSync(xdgConfigHome, { recursive: true });

  const desktopEntry = ['[Desktop Entry]', `Name=${displayName}`, `Exec=${execCommand}`];
  if (iconPath) {
    desktopEntry.push(`Icon=${iconPath}`);
  }
  desktopEntry.push('Type=Application', `MimeType=${mimeType};`);

  fs.writeFileSync(path.join(appsDir, desktopFileId), desktopEntry.join('\n'));
  fs.writeFileSync(
    path.join(xdgConfigHome, 'mimeapps.list'),
    ['[Default Applications]', `${mimeType}=${desktopFileId}`].join('\n')
  );
}

function spawnProtocolLookupWithXdgMock(
  url: string,
  xdgDataHome: string,
  xdgConfigHome: string,
  options: {
    lookupMode?: 'info';
    extraEnv?: Record<string, string | undefined>;
  } = {}
): Promise<ProtocolNameLookupResult | ProtocolInfoLookupResult> {
  const { lookupMode, extraEnv = {} } = options;

  return new Promise((resolve, reject) => {
    const env: NodeJS.ProcessEnv = {
      ...process.env,
      ...extraEnv,
      XDG_DATA_HOME: xdgDataHome,
      XDG_DATA_DIRS: [xdgDataHome, process.env.XDG_DATA_DIRS].filter(Boolean).join(':'),
      XDG_CONFIG_HOME: xdgConfigHome
    };
    if (lookupMode === 'info') {
      env.ELECTRON_PROTOCOL_LOOKUP_MODE = 'info';
    }

    const child = cp.spawn(process.execPath, [protocolLookupFixturePath, url], {
      env,
      stdio: ['ignore', 'pipe', 'pipe']
    });
    let stdout = '';
    let stderr = '';
    child.stdout.on('data', (data: Buffer) => {
      stdout += data;
    });
    child.stderr.on('data', (data: Buffer) => {
      stderr += data;
    });
    child.on('close', (code) => {
      if (code !== 0) {
        reject(new Error(`Fixture exited with code ${code}: ${stderr}`));
        return;
      }

      try {
        resolve(JSON.parse(stdout));
      } catch {
        reject(new Error(`Failed to parse output: ${stdout}\nstderr: ${stderr}`));
      }
    });
    child.on('error', reject);
  });
}

export async function spawnProtocolNameWithXdgMock(
  url: string,
  xdgDataHome: string,
  xdgConfigHome: string,
  extraEnv: Record<string, string | undefined> = {}
) {
  const parsed = (await spawnProtocolLookupWithXdgMock(url, xdgDataHome, xdgConfigHome, {
    extraEnv
  })) as ProtocolNameLookupResult;
  return parsed.name;
}

export function spawnProtocolInfoWithXdgMock(
  url: string,
  xdgDataHome: string,
  xdgConfigHome: string,
  extraEnv: Record<string, string | undefined> = {}
) {
  return spawnProtocolLookupWithXdgMock(url, xdgDataHome, xdgConfigHome, {
    lookupMode: 'info',
    extraEnv
  }) as Promise<ProtocolInfoLookupResult>;
}
