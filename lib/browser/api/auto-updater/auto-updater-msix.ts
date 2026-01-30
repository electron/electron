import * as msixUpdate from '@electron/internal/browser/api/auto-updater/msix-update-win';

import { app, net } from 'electron/main';

import { EventEmitter } from 'events';

interface UpdateInfo {
  ok: boolean; // False if error encountered
  available?: boolean; // True if the update is available, false if not
  updateUrl?: string; // The URL of the update
  releaseNotes?: string; // The release notes of the update
  releaseName?: string; // The release name of the update
  releaseDate?: Date; // The release date of the update
}

interface MSIXPackageInfo {
  id: string;
  familyName: string;
  developmentMode: boolean;
  version: string;
  signatureKind: 'developer' | 'enterprise' | 'none' | 'store' | 'system';
  appInstallerUri?: string;
}

/**
 * Options for updating an MSIX package.
 * Used with `updateMsix()` to control how the package update behaves.
 *
 * These options correspond to the Windows.Management.Deployment.AddPackageOptions class properties.
 *
 * @see https://learn.microsoft.com/en-us/uwp/api/windows.management.deployment.addpackageoptions?view=winrt-26100
 */
export interface UpdateMsixOptions {
  /**
   * Gets or sets a value that indicates whether to delay registration of the main package
   * or dependency packages if the packages are currently in use.
   *
   * Corresponds to `AddPackageOptions.DeferRegistrationWhenPackagesAreInUse`
   *
   * @default false
   */
  deferRegistration?: boolean;

  /**
   * Gets or sets a value that indicates whether the app is installed in developer mode.
   * When set, the app is installed in development mode which allows for a more rapid
   * development cycle. The BlockMap.xml, [Content_Types].xml, and digital signature
   * files are not required for app installation.
   *
   * Corresponds to `AddPackageOptions.DeveloperMode`
   *
   * @default false
   */
  developerMode?: boolean;

  /**
   * Gets or sets a value that indicates whether the processes associated with the package
   * will be shut down forcibly so that registration can continue if the package, or any
   * package that depends on the package, is currently in use.
   *
   * Corresponds to `AddPackageOptions.ForceAppShutdown`
   *
   * @default false
   */
  forceShutdown?: boolean;

  /**
   * Gets or sets a value that indicates whether the processes associated with the package
   * will be shut down forcibly so that registration can continue if the package is
   * currently in use.
   *
   * Corresponds to `AddPackageOptions.ForceTargetAppShutdown`
   *
   * @default false
   */
  forceTargetShutdown?: boolean;

  /**
   * Gets or sets a value that indicates whether to force a specific version of a package
   * to be added, regardless of if a higher version is already added.
   *
   * Corresponds to `AddPackageOptions.ForceUpdateFromAnyVersion`
   *
   * @default false
   */
  forceUpdateFromAnyVersion?: boolean;
}

/**
 * Options for registering an MSIX package.
 * Used with `registerPackage()` to control how the package registration behaves.
 *
 * These options correspond to the Windows.Management.Deployment.DeploymentOptions enum.
 *
 * @see https://learn.microsoft.com/en-us/uwp/api/windows.management.deployment.deploymentoptions?view=winrt-26100
 */
interface RegisterPackageOptions {
  /**
   * Force shutdown of the application if it's currently running.
   * If this package, or any package that depends on this package, is currently in use,
   * the processes associated with the package are shut down forcibly so that registration can continue.
   *
   * Corresponds to `DeploymentOptions.ForceApplicationShutdown` (value: 1)
   *
   * @default false
   */
  forceShutdown?: boolean;

  /**
   * Force shutdown of the target application if it's currently running.
   * If this package is currently in use, the processes associated with the package
   * are shut down forcibly so that registration can continue.
   *
   * Corresponds to `DeploymentOptions.ForceTargetApplicationShutdown` (value: 64)
   *
   * @default false
   */
  forceTargetShutdown?: boolean;

  /**
   * Force a specific version of a package to be staged/registered, regardless of if
   * a higher version is already staged/registered.
   *
   * Corresponds to `DeploymentOptions.ForceUpdateFromAnyVersion` (value: 262144)
   *
   * @default false
   */
  forceUpdateFromAnyVersion?: boolean;
}

class AutoUpdater extends EventEmitter implements Electron.AutoUpdater {
  updateAvailable: boolean = false;
  updateURL: string | null = null;
  updateHeaders: Record<string, string> | null = null;
  allowAnyVersion: boolean = false;

  // Private: Validate that the URL points to an MSIX file (following redirects)
  private async validateMsixUrl (url: string): Promise<void> {
    try {
      // Make a HEAD request to follow redirects and get the final URL
      const response = await net.fetch(url, {
        method: 'HEAD',
        headers: this.updateHeaders ? new Headers(this.updateHeaders) : undefined,
        redirect: 'follow' // Follow redirects to get the final URL
      });

      // Get the final URL after redirects (response.url contains the final URL)
      const finalUrl = response.url || url;
      const urlObj = new URL(finalUrl);
      const pathname = urlObj.pathname.toLowerCase();

      // Check if final URL ends with .msix or .msixbundle extension
      const hasMsixExtension = pathname.endsWith('.msix') || pathname.endsWith('.msixbundle');

      if (!hasMsixExtension) {
        throw new Error(`Update URL does not point to an MSIX file. Expected .msix or .msixbundle extension, got final URL: ${finalUrl}`);
      }
    } catch (error) {
      if (error instanceof TypeError) {
        throw new Error(`Invalid MSIX URL: ${url}`);
      }
      throw error;
    }
  }

  // Private: Check if URL is a direct MSIX file (following redirects)
  private async isDirectMsixUrl (url: string, emitError: boolean = false): Promise<boolean> {
    try {
      await this.validateMsixUrl(url);
      return true;
    } catch (error) {
      if (emitError) {
        this.emitError(error as Error);
      }
      return false;
    }
  }

  // Supports both  versioning (x.y.z) and Windows version format (x.y.z.a)
  // Returns: 1 if v1 > v2, -1 if v1 < v2, 0 if v1 === v2
  private compareVersions (v1: string, v2: string): number {
    const parts1 = v1.split('.').map(part => {
      const parsed = parseInt(part, 10);
      return isNaN(parsed) ? 0 : parsed;
    });
    const parts2 = v2.split('.').map(part => {
      const parsed = parseInt(part, 10);
      return isNaN(parsed) ? 0 : parsed;
    });

    const maxLength = Math.max(parts1.length, parts2.length);

    for (let i = 0; i < maxLength; i++) {
      const part1 = parts1[i] ?? 0;
      const part2 = parts2[i] ?? 0;

      if (part1 > part2) return 1;
      if (part1 < part2) return -1;
    }

    return 0;
  }

  // Private: Parse the static releases array format
  // This is a static JSON file containing all releases
  private parseStaticReleasFile (json: any, currentVersion: string): { ok: boolean; available: boolean; url?: string; name?: string; notes?: string; pub_date?: string } {
    if (!Array.isArray(json.releases) || !json.currentRelease || typeof json.currentRelease !== 'string') {
      this.emitError(new Error('Invalid releases format. Expected \'releases\' array and \'currentRelease\' string.'));
      return { ok: false, available: false };
    }

    // Use currentRelease property to determine if update is available
    const currentReleaseVersion = json.currentRelease;

    // Compare current version with currentRelease
    const versionComparison = this.compareVersions(currentReleaseVersion, currentVersion);

    // If versions match, we're up to date
    if (versionComparison === 0) {
      return { ok: true, available: false };
    }

    // If currentRelease is older than current version, check allowAnyVersion
    if (versionComparison < 0) {
      // If allowAnyVersion is true, allow downgrades
      if (this.allowAnyVersion) {
        // Continue to find the release entry for downgrade
      } else {
        return { ok: true, available: false };
      }
    }

    // currentRelease is newer, find the release entry
    const releaseEntry = json.releases.find((r: any) => r.version === currentReleaseVersion);

    if (!releaseEntry || !releaseEntry.updateTo) {
      this.emitError(new Error(`Release entry for version '${currentReleaseVersion}' not found or missing 'updateTo' property.`));
      return { ok: false, available: false };
    }

    const updateTo = releaseEntry.updateTo;

    if (!updateTo.url) {
      this.emitError(new Error(`Invalid release entry. 'updateTo.url' is missing for version ${currentReleaseVersion}.`));
      return { ok: false, available: false };
    }

    return {
      ok: true,
      available: true,
      url: updateTo.url,
      name: updateTo.name,
      notes: updateTo.notes,
      pub_date: updateTo.pub_date
    };
  }

  private parseDynamicReleasFile (json: any): { ok: boolean; available: boolean; url?: string; name?: string; notes?: string; pub_date?: string } {
    if (!json.url) {
      this.emitError(new Error('Invalid releases format. Expected \'url\' string property.'));
      return { ok: false, available: false };
    }
    return { ok: true, available: true, url: json.url, name: json.name, notes: json.notes, pub_date: json.pub_date };
  }

  private async fetchSquirrelJson (url: string) {
    const headers: Record<string, string> = {
      ...this.updateHeaders,
      Accept: 'application/json' // Always set Accept header, overriding any user-provided Accept
    };
    const response = await net.fetch(url, {
      headers
    });

    if (response.status === 204) {
      return { ok: true, available: false };
    } else if (response.status === 200) {
      const updateJson = await response.json();

      // Check if this is the static releases array format
      if (Array.isArray(updateJson.releases)) {
        // Get current package version
        const packageInfo = msixUpdate.getPackageInfo();
        const currentVersion = packageInfo.version;

        if (!currentVersion) {
          this.emitError(new Error('Cannot determine current package version.'));
          return { ok: false, available: false };
        }

        return this.parseStaticReleasFile(updateJson, currentVersion);
      } else {
        // Dynamic format: server returns JSON with update info for current version
        return this.parseDynamicReleasFile(updateJson);
      }
    } else {
      this.emitError(new Error(`Unexpected response status: ${response.status}`));
      return { ok: false, available: false };
    }
  }

  private async getUpdateInfo (url: string): Promise<UpdateInfo> {
    if (url && await this.isDirectMsixUrl(url)) {
      return { ok: true, available: true, updateUrl: url, releaseDate: new Date() };
    } else {
      const updateJson = await this.fetchSquirrelJson(url);
      if (!updateJson.ok) {
        return { ok: false };
      } else if (updateJson.ok && !updateJson.available) {
        return { ok: true, available: false };
      } else {
        // updateJson.ok && updateJson.available must be true here
        // Parse the publication date if present (ISO 8601 format)
        let releaseDate: Date | null = null;
        if (updateJson.pub_date) {
          releaseDate = new Date(updateJson.pub_date);
        }

        const updateUrl = updateJson.url ?? '';
        const releaseNotes = updateJson.notes ?? '';
        const releaseName = updateJson.name ?? '';
        releaseDate = releaseDate ?? new Date();

        if (!await this.isDirectMsixUrl(updateUrl, true)) {
          return { ok: false };
        } else {
          return {
            ok: true,
            available: true,
            updateUrl,
            releaseNotes,
            releaseName,
            releaseDate
          };
        }
      }
    }
  }

  getFeedURL () {
    return this.updateURL ?? '';
  }

  setFeedURL (options: { url: string; headers?: Record<string, string>; allowAnyVersion?: boolean } | string) {
    let updateURL: string;
    let headers: Record<string, string> | undefined;
    let allowAnyVersion: boolean | undefined;
    if (typeof options === 'object') {
      if (typeof options.url === 'string') {
        updateURL = options.url;
        headers = options.headers;
        allowAnyVersion = options.allowAnyVersion;
      } else {
        throw new TypeError('Expected options object to contain a \'url\' string property in setFeedUrl call');
      }
    } else if (typeof options === 'string') {
      updateURL = options;
    } else {
      throw new TypeError('Expected an options object with a \'url\' property to be provided');
    }
    this.updateURL = updateURL;
    this.updateHeaders = headers ?? null;
    this.allowAnyVersion = allowAnyVersion ?? false;
  }

  getPackageInfo (): MSIXPackageInfo {
    return msixUpdate.getPackageInfo() as MSIXPackageInfo;
  }

  async checkForUpdates () {
    const url = this.updateURL;
    if (!url) {
      return this.emitError(new Error('Update URL is not set'));
    }

    // Check if running in MSIX package
    const packageInfo = msixUpdate.getPackageInfo();
    if (!packageInfo.familyName) {
      return this.emitError(new Error('MSIX updates are not supported'));
    }

    // If appInstallerUri is set, Windows App Installer manages updates automatically
    // Prevent updates here to avoid conflicts
    if (packageInfo.appInstallerUri) {
      return this.emitError(new Error('Auto-updates are managed by Windows App Installer. Updates are not allowed when installed via Application Manifest.'));
    }

    this.emit('checking-for-update');
    try {
      const msixUrlInfo = await this.getUpdateInfo(url);
      if (!msixUrlInfo.ok) {
        return this.emitError(new Error('Invalid update or MSIX URL. See previous errors.'));
      }

      if (!msixUrlInfo.available) {
        this.emit('update-not-available');
      } else {
        this.updateAvailable = true;
        this.emit('update-available');
        await msixUpdate.updateMsix(msixUrlInfo.updateUrl, {
          deferRegistration: true,
          developerMode: false,
          forceShutdown: false,
          forceTargetShutdown: false,
          forceUpdateFromAnyVersion: this.allowAnyVersion
        } as UpdateMsixOptions);

        this.emit('update-downloaded', {}, msixUrlInfo.releaseNotes, msixUrlInfo.releaseName, msixUrlInfo.releaseDate, msixUrlInfo.updateUrl, () => {
          this.quitAndInstall();
        });
      }
    } catch (error) {
      this.emitError(error as Error);
    }
  }

  async quitAndInstall () {
    if (!this.updateAvailable) {
      this.emitError(new Error('No update available, can\'t quit and install'));
      app.quit();
      return;
    }

    try {
      // Get package info to get family name
      const packageInfo = msixUpdate.getPackageInfo();
      if (!packageInfo.familyName) {
        return this.emitError(new Error('MSIX updates are not supported'));
      }

      msixUpdate.registerRestartOnUpdate('');
      this.emit('before-quit-for-update');
      // force shutdown of the application and register the package to be installed on restart
      await msixUpdate.registerPackage(packageInfo.familyName, {
        forceShutdown: true
      } as RegisterPackageOptions);
    } catch (error) {
      this.emitError(error as Error);
    }
  }

  // Private: Emit both error object and message, this is to keep compatibility
  // with Old APIs.
  emitError (error: Error) {
    this.emit('error', error, error.message);
  }
}

export default new AutoUpdater();
