import * as cp from 'node:child_process';
import * as path from 'node:path';

import { ifdescribe } from './lib/spec-helpers';

ifdescribe(process.platform === 'darwin' && process.mas)('Mac App Store build', () => {
  describe('private API usage', () => {
    // Get paths to all Electron binaries
    const getElectronBinaries = () => {
      const contentsPath = path.dirname(path.dirname(process.execPath));
      return {
        mainProcess: process.execPath,
        framework: path.join(contentsPath, 'Frameworks', 'Electron Framework.framework', 'Electron Framework'),
        helpers: {
          main: path.join(contentsPath, 'Frameworks', 'Electron Helper.app', 'Contents', 'MacOS', 'Electron Helper'),
          gpu: path.join(contentsPath, 'Frameworks', 'Electron Helper (GPU).app', 'Contents', 'MacOS', 'Electron Helper (GPU)'),
          plugin: path.join(contentsPath, 'Frameworks', 'Electron Helper (Plugin).app', 'Contents', 'MacOS', 'Electron Helper (Plugin)'),
          renderer: path.join(contentsPath, 'Frameworks', 'Electron Helper (Renderer).app', 'Contents', 'MacOS', 'Electron Helper (Renderer)')
        }
      };
    };

    const checkBinaryForPrivateAPIs = (binaryPath: string, binaryName: string) => {
      // Use nm without -U to get all symbols (both defined and undefined)
      const nmResult = cp.spawnSync('nm', ['-g', binaryPath], {
        encoding: 'utf-8',
        maxBuffer: 50 * 1024 * 1024
      });

      if (nmResult.error) {
        throw new Error(`Failed to run nm on ${binaryName}: ${nmResult.error.message}`);
      }

      const symbols = nmResult.stdout;

      // List of known private APIs that should not be present in MAS builds
      // These are from the mas_avoid_private_macos_api_usage.patch
      const privateAPIs = [
        'abort_report_np',
        'pthread_fchdir_np',
        'pthread_chdir_np',
        'SetApplicationIsDaemon',
        '_LSSetApplicationLaunchServicesServerConnectionStatus',
        'CGSSetWindowCaptureExcludeShape',
        'CGRegionCreateWithRect',
        'CTFontCopyVariationAxesInternal',
        'AudioDeviceDuck',
        'CGSMainConnectionID',
        'IOBluetoothPreferenceSetControllerPowerState',
        'CGSSetDenyWindowServerConnections',
        'CGFontRenderingGetFontSmoothingDisabled',
        'CTFontDescriptorIsSystemUIFont',
        'sandbox_init_with_parameters',
        'sandbox_create_params',
        'sandbox_set_param',
        'sandbox_free_params',
        'sandbox_compile_string',
        'sandbox_apply',
        'sandbox_free_profile',
        'sandbox_extension_issue_file',
        'sandbox_extension_consume',
        'sandbox_extension_release',
        '_CSCheckFixDisable',
        'responsibility_get_pid_responsible_for_pid',
        '_NSAppendToKillRing',
        '_NSPrependToKillRing',
        '_NSYankFromKillRing',
        '__NSNewKillRingSequence',
        '__NSInitializeKillRing',
        '_NSSetKillRingToYankedState'
      ];

      const foundPrivateAPIs: string[] = [];

      for (const api of privateAPIs) {
        // Check if the symbol appears in the nm output
        // Look for ' U ' (undefined) or ' T ' (defined in text section) followed by the API
        // nm output format is like: "                 U _symbol_name"
        const regex = new RegExp(`\\s+[UT]\\s+(_${api})\\b`);
        const match = symbols.match(regex);
        if (match) {
          // Keep the full symbol name including the underscore
          foundPrivateAPIs.push(match[1]);
        }
      }

      return foundPrivateAPIs;
    };

    it('should not use private macOS APIs in main process', function () {
      this.timeout(60000);

      const binaries = getElectronBinaries();
      const foundPrivateAPIs = checkBinaryForPrivateAPIs(binaries.mainProcess, 'Electron main process');

      if (foundPrivateAPIs.length > 0) {
        throw new Error(
          `Found private macOS APIs in main process:\n${foundPrivateAPIs.join('\n')}\n\n` +
          'These APIs are not allowed in Mac App Store builds and will cause rejection.'
        );
      }

      // Also check for private framework linkage
      const otoolResult = cp.spawnSync('otool', ['-L', binaries.mainProcess], {
        encoding: 'utf-8'
      });

      if (otoolResult.error) {
        throw new Error(`Failed to run otool: ${otoolResult.error.message}`);
      }

      const frameworks = otoolResult.stdout;

      if (frameworks.includes('PrivateFrameworks')) {
        throw new Error(
          'Found linkage to PrivateFrameworks which is not allowed in Mac App Store builds:\n' +
          frameworks
        );
      }
    });

    it('should not use private macOS APIs in Electron Framework', function () {
      this.timeout(60000);

      // Check the Electron Framework binary (mentioned in issue #49616)
      const binaries = getElectronBinaries();
      const foundAPIs = checkBinaryForPrivateAPIs(binaries.framework, 'Electron Framework');

      if (foundAPIs.length > 0) {
        throw new Error(
          `Found private macOS APIs in Electron Framework:\n${foundAPIs.join('\n')}\n\n` +
          'These APIs are not allowed in Mac App Store builds and will cause rejection.\n' +
          'See patches/chromium/mas_avoid_private_macos_api_usage.patch.patch'
        );
      }
    });

    it('should not use private macOS APIs in helper processes', function () {
      this.timeout(60000);

      const binaries = getElectronBinaries();
      const allFoundAPIs: Record<string, string[]> = {};

      for (const [helperName, helperPath] of Object.entries(binaries.helpers)) {
        const displayName = `Electron Helper${helperName !== 'main' ? ` (${helperName.charAt(0).toUpperCase() + helperName.slice(1)})` : ''}`;
        const foundAPIs = checkBinaryForPrivateAPIs(helperPath, displayName);

        if (foundAPIs.length > 0) {
          allFoundAPIs[displayName] = foundAPIs;
        }
      }

      if (Object.keys(allFoundAPIs).length > 0) {
        const errorLines = Object.entries(allFoundAPIs).map(([helper, apis]) =>
          `${helper}:\n  ${apis.join('\n  ')}`
        );

        throw new Error(
          `Found private macOS APIs in helper processes:\n\n${errorLines.join('\n\n')}\n\n` +
          'These APIs are not allowed in Mac App Store builds and will cause rejection.'
        );
      }
    });

    it('should not reference private Objective-C classes', function () {
      this.timeout(60000);

      // Check for private Objective-C classes (appear as _OBJC_CLASS_$_ClassName)
      const privateClasses = [
        'NSAccessibilityRemoteUIElement',
        'CAContext'
      ];

      const binaries = getElectronBinaries();
      const binariesToCheck = [
        { path: binaries.mainProcess, name: 'Electron main process' },
        { path: binaries.framework, name: 'Electron Framework' },
        { path: binaries.helpers.main, name: 'Electron Helper' },
        { path: binaries.helpers.renderer, name: 'Electron Helper (Renderer)' }
      ];

      const foundClasses: Record<string, string[]> = {};

      for (const binary of binariesToCheck) {
        const nmResult = cp.spawnSync('nm', ['-g', binary.path], {
          encoding: 'utf-8',
          maxBuffer: 50 * 1024 * 1024
        });

        if (nmResult.error) {
          throw new Error(`Failed to run nm on ${binary.name}: ${nmResult.error.message}`);
        }

        const symbols = nmResult.stdout;
        const foundInBinary: string[] = [];

        for (const className of privateClasses) {
          // Look for _OBJC_CLASS_$_ClassName pattern
          if (symbols.includes(`_OBJC_CLASS_$_${className}`)) {
            foundInBinary.push(className);
          }
        }

        if (foundInBinary.length > 0) {
          foundClasses[binary.name] = foundInBinary;
        }
      }

      if (Object.keys(foundClasses).length > 0) {
        const errorLines = Object.entries(foundClasses).map(([binary, classes]) =>
          `${binary}:\n  ${classes.join('\n  ')}`
        );

        throw new Error(
          `Found references to private Objective-C classes:\n\n${errorLines.join('\n\n')}\n\n` +
          'These are not allowed in Mac App Store builds and will cause rejection.'
        );
      }
    });
  });
});
