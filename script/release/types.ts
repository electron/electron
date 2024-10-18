export const ELECTRON_ORG = 'electron';
export const ELECTRON_REPO = 'electron';
export const NIGHTLY_REPO = 'nightlies';

export type ElectronReleaseRepo = 'electron' | 'nightlies';

export type VersionBumpType = 'nightly' | 'alpha' | 'beta' | 'minor' | 'stable';

export const isVersionBumpType = (s: string): s is VersionBumpType => {
  return ['nightly', 'alpha', 'beta', 'minor', 'stable'].includes(s);
};
