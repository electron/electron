const { app, autoUpdater } = require('electron');

// Parse command-line arguments
const args = process.argv.slice(2);
const command = args[0];
const commandArg = args[1];

app.whenReady().then(() => {
  try {
    // Debug: log received arguments
    if (process.env.DEBUG) {
      console.log('Command:', command);
      console.log('Command arg:', commandArg);
      console.log('All args:', JSON.stringify(args));
    }

    if (command === '--printPackageId') {
      const packageInfo = autoUpdater.getPackageInfo();
      if (packageInfo.familyName) {
        console.log(`Family Name: ${packageInfo.familyName}`);
        console.log(`Package ID: ${packageInfo.id || 'N/A'}`);
        console.log(`Version: ${packageInfo.version || 'N/A'}`);
        console.log(`Development Mode: ${packageInfo.developmentMode ? 'Yes' : 'No'}`);
        console.log(`Signature Kind: ${packageInfo.signatureKind || 'N/A'}`);
        if (packageInfo.appInstallerUri) {
          console.log(`App Installer URI: ${packageInfo.appInstallerUri}`);
        }
        app.quit();
      } else {
        console.error('No package identity found. Process is not running in an MSIX package context.');
        app.exit(1);
      }
    } else if (command === '--checkUpdate') {
      if (!commandArg) {
        console.error('Update URL is required for --checkUpdate');
        app.exit(1);
        return;
      }

      // Use hardcoded headers if --useCustomHeaders flag is provided
      let headers;
      let allowAnyVersion = false;
      if (args[2] === '--useCustomHeaders') {
        headers = {
          'X-AppVersion': '1.0.0',
          Authorization: 'Bearer test-token'
        };
      } else if (args[2] === '--allowAnyVersion') {
        allowAnyVersion = true;
      }

      // Set up event listeners
      autoUpdater.on('checking-for-update', () => {
        console.log('Checking for update...');
      });

      autoUpdater.on('update-available', () => {
        console.log('Update available');
      });

      autoUpdater.on('update-not-available', () => {
        console.log('Update not available');
        app.quit();
      });

      autoUpdater.on('update-downloaded', (event, releaseNotes, releaseName, releaseDate, updateUrl) => {
        console.log('Update downloaded');
        console.log(`Release Name: ${releaseName || 'N/A'}`);
        console.log(`Release Notes: ${releaseNotes || 'N/A'}`);
        console.log(`Release Date: ${releaseDate || 'N/A'}`);
        console.log(`Update URL: ${updateUrl || 'N/A'}`);
        app.quit();
      });

      autoUpdater.on('error', (error, message) => {
        console.error(`Update error: ${message || error.message || 'Unknown error'}`);
        app.exit(1);
      });

      // Set the feed URL with optional headers and allowAnyVersion, then check for updates
      if (headers || allowAnyVersion) {
        autoUpdater.setFeedURL({ url: commandArg, headers, allowAnyVersion });
      } else {
        autoUpdater.setFeedURL(commandArg);
      }
      autoUpdater.checkForUpdates();
    } else {
      console.error(`Unknown command: ${command || '(none)'}`);
      app.exit(1);
    }
  } catch (error) {
    console.error('Unhandled error:', error.message);
    console.error(error.stack);
    app.exit(1);
  }
});
