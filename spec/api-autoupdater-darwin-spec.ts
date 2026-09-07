import { expect } from 'chai';

import { setupUpdaterHarness, shouldRunUpdaterSpecs } from './lib/autoupdater-darwin-helpers';
import { copyMacOSFixtureApp, unsignApp } from './lib/codesign-helpers';
import { withTempDirectory } from './lib/fs-helpers';
import { RoutedResponse } from './lib/http-server-helpers';
import { ifdescribe, ifit } from './lib/spec-helpers';

// The update lifecycle: checking, downloading, staging, installing and
// relaunching. The rules for refusing or altering an update are in
// api-autoupdater-darwin-policy-spec.ts; the two files share a harness and
// are split so that they can land on different CI shards.
ifdescribe(shouldRunUpdaterSpecs)('autoUpdater behavior', function () {
  this.timeout(120000);

  const harness = setupUpdaterHarness();
  const { launchApp, shallowSign, logOnError, updaterIt } = harness;

  it('should have a valid code signing identity', () => {
    expect(harness.identity()).to.be.a('string').with.lengthOf.at.least(1);
  });

  // These use the raw build output, not the template. On arm64 builds the
  // built app is self-signed by default so the setFeedURL call always works.
  ifit(process.arch !== 'arm64')('should fail to set the feed URL when the app is not signed', async () => {
    await withTempDirectory(async (dir) => {
      const appPath = await copyMacOSFixtureApp(dir);
      await unsignApp(appPath);
      const launchResult = await launchApp(appPath, ['http://myupdate']);
      console.log(launchResult);
      expect(launchResult.code).to.equal(1);
      expect(launchResult.out).to.include('Could not get code signature for running application');
    });
  });

  ifit(process.arch !== 'arm64')(
    'should fail with code signature error when serverType is default and app is unsigned',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        await unsignApp(appPath);
        const launchResult = await launchApp(appPath, ['', 'default']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include('Could not get code signature for running application');
      });
    }
  );

  ifit(process.arch !== 'arm64')(
    'should fail with code signature error when serverType is json and app is unsigned',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        await unsignApp(appPath);
        const launchResult = await launchApp(appPath, ['', 'json']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include('Could not get code signature for running application');
      });
    }
  );

  ifit(process.arch !== 'arm64')(
    'should fail with serverType error when an invalid serverType is provided',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        const launchResult = await launchApp(appPath, ['', 'weow']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include("Expected serverType to be 'default' or 'json'");
      });
    }
  );

  it('should cleanly set the feed URL when the app is signed', async () => {
    await withTempDirectory(async (dir) => {
      const appPath = await copyMacOSFixtureApp(dir, 'initial', { sourceApp: harness.templateApp() });
      await shallowSign(appPath);
      const launchResult = await launchApp(appPath, ['http://myupdate']);
      expect(launchResult.code).to.equal(0);
      expect(launchResult.out).to.include('Feed URL Set: http://myupdate');
    });
  });

  describe('with update server', () => {
    updaterIt('should hit the update endpoint when checkForUpdates is called', async (ctx) => {
      await withTempDirectory(async (dir) => {
        const appPath = await ctx.copySignedApp(dir, 'check');
        ctx.server.get('/update-check', (req, res) => {
          res.status(204).send();
        });
        const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
        logOnError(launchResult, () => {
          expect(launchResult.code).to.equal(0);
          expect(ctx.requests).to.have.lengthOf(1);
          expect(ctx.requests[0]).to.have.property('url', '/update-check');
          expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
        });
      });
    });

    updaterIt('should hit the update endpoint with customer headers when checkForUpdates is called', async (ctx) => {
      await withTempDirectory(async (dir) => {
        const appPath = await ctx.copySignedApp(dir, 'check-with-headers');
        ctx.server.get('/update-check', (req, res) => {
          res.status(204).send();
        });
        const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
        logOnError(launchResult, () => {
          expect(launchResult.code).to.equal(0);
          expect(ctx.requests).to.have.lengthOf(1);
          expect(ctx.requests[0]).to.have.property('url', '/update-check');
          expect(ctx.requests[0].header('x-test')).to.equal('this-is-a-test');
        });
      });
    });

    updaterIt(
      'should hit the download endpoint when an update is available and error if the file is bad',
      async (ctx) => {
        await withTempDirectory(async (dir) => {
          const appPath = await ctx.copySignedApp(dir, 'update');
          ctx.server.get('/update-file', (req, res) => {
            res.status(500).send('This is not a file');
          });
          ctx.server.get('/update-check', (req, res) => {
            res.json({
              url: `http://localhost:${ctx.port}/update-file`,
              name: 'My Release Name',
              notes: 'Theses are some release notes innit',
              pub_date: new Date().toString()
            });
          });
          const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
          logOnError(launchResult, () => {
            expect(launchResult).to.have.property('code', 1);
            expect(launchResult.out).to.include('Update download failed. The server sent an invalid response.');
            expect(ctx.requests).to.have.lengthOf(2);
            expect(ctx.requests[0]).to.have.property('url', '/update-check');
            expect(ctx.requests[1]).to.have.property('url', '/update-file');
            expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
            expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
          });
        });
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update'
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(3);
            expect(ctx.requests[2].url).to.equal('/update-check/updated/2.0.0');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided even after a different update was staged',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-stack',
            endFixture: 'update-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-stack');
            let updateCount = 0;
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateCount > 1 ? updateZipPath3 : updateZipPath2);
            });
            ctx.server.get('/update-check', (req, res) => {
              updateCount++;
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(4);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[2]).to.have.property('url', '/update-check');
              expect(ctx.requests[3]).to.have.property('url', '/update-file');
              expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[3].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(5);
            expect(ctx.requests[4].url).to.equal('/update-check/updated/3.0.0');
            expect(ctx.requests[4].header('user-agent')).to.include('Electron/');
          }
        );
      },
      { timeout: 180000 }
    );

    updaterIt(
      'should preserve the staged update directory and prune orphaned ones when a new update is downloaded',
      async (ctx) => {
        // Clean up any existing update directories before the test
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-stack',
            endFixture: 'update-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-stack');
            let updateCount = 0;
            let downloadCount = 0;
            let dirsDuringFirstDownload: string[] = [];
            let dirsDuringSecondDownload: string[] = [];

            ctx.server.get('/update-file', async (req, res) => {
              downloadCount++;
              // Snapshot update directories at the moment each download begins.
              // By this point uniqueTemporaryDirectoryForUpdate has already run
              // (prune + mkdtemp). We want to verify:
              //   1st download: 1 dir (nothing to preserve, nothing to prune)
              //   2nd download: 2 dirs (staged dir from 1st check is preserved
              //                 so quitAndInstall stays safe, + new temp dir)
              // The count never exceeds 2 across repeated checks — orphaned dirs
              // (no longer referenced by ShipItState.plist) get pruned.
              if (downloadCount === 1) {
                dirsDuringFirstDownload = await ctx.getUpdateDirectoriesInCache();
              } else if (downloadCount === 2) {
                dirsDuringSecondDownload = await ctx.getUpdateDirectoriesInCache();
              }
              res.download(updateCount > 1 ? updateZipPath3 : updateZipPath2);
            });
            ctx.server.get('/update-check', (req, res) => {
              updateCount++;
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
            });

            await relaunchPromise;

            // First download: exactly one temp dir (the first update).
            expect(dirsDuringFirstDownload).to.have.lengthOf(
              1,
              `Expected 1 update directory during first download but found ${dirsDuringFirstDownload.length}: ${dirsDuringFirstDownload.join(', ')}`
            );

            // Second download: exactly two — the staged one preserved + the new
            // one. Crucially the first download's directory must still be present,
            // otherwise a mid-download quitAndInstall would find a dangling
            // ShipItState.plist.
            expect(dirsDuringSecondDownload).to.have.lengthOf(
              2,
              `Expected 2 update directories during second download (staged + new) but found ${dirsDuringSecondDownload.length}: ${dirsDuringSecondDownload.join(', ')}`
            );
            expect(dirsDuringSecondDownload).to.include(
              dirsDuringFirstDownload[0],
              'The staged update directory from the first download must be preserved during the second download'
            );
          }
        );
      }
    );

    updaterIt(
      'should keep the update directory count bounded across repeated checks',
      async (ctx) => {
        // Verifies the orphan prune actually fires: after a second download
        // completes and rewrites ShipItState.plist, the first directory is no
        // longer referenced and must be removed when a third check begins.
        // Without this, directories would accumulate forever.
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-triple-stack',
            endFixture: 'update-triple-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-triple-stack');
            const updateZipPath4 = await ctx.getUpdateZip('4.0.0', 'update-triple-stack');
            let downloadCount = 0;
            const dirsPerDownload: string[][] = [];

            ctx.server.get('/update-file', async (req, res) => {
              downloadCount++;
              // Snapshot after prune+mkdtemp but before the payload transfers.
              dirsPerDownload.push(await ctx.getUpdateDirectoriesInCache());
              const zips = [updateZipPath2, updateZipPath3, updateZipPath4];
              res.download(zips[Math.min(downloadCount, zips.length) - 1]);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();

            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
            });

            await relaunchPromise;
            expect(ctx.requests[ctx.requests.length - 1].url).to.equal('/update-check/updated/4.0.0');

            expect(dirsPerDownload).to.have.lengthOf(3);

            // 1st: fresh cache, 1 dir.
            expect(dirsPerDownload[0]).to.have.lengthOf(1, `1st download: ${dirsPerDownload[0].join(', ')}`);

            // 2nd: staged (1st) preserved + new = 2 dirs.
            expect(dirsPerDownload[1]).to.have.lengthOf(2, `2nd download: ${dirsPerDownload[1].join(', ')}`);
            expect(dirsPerDownload[1]).to.include(dirsPerDownload[0][0]);

            // 3rd: 1st is now orphaned (plist points to 2nd) — must be pruned.
            // Staged (2nd) preserved + new = still 2 dirs. Bounded.
            expect(dirsPerDownload[2]).to.have.lengthOf(2, `3rd download: ${dirsPerDownload[2].join(', ')}`);
            expect(dirsPerDownload[2]).to.not.include(
              dirsPerDownload[0][0],
              'The first (now orphaned) update directory must be pruned on the third check'
            );
            const secondDir = dirsPerDownload[1].find((d) => d !== dirsPerDownload[0][0]);
            expect(dirsPerDownload[2]).to.include(
              secondDir,
              'The second (currently staged) update directory must be preserved on the third check'
            );
          }
        );
      },
      { timeout: 240000 }
    );

    // Regression test for https://github.com/electron/electron/issues/50200
    //
    // When checkForUpdates() is called again after an update has been staged,
    // Squirrel creates a new temporary directory and prunes old ones. If the
    // prune removes the directory that ShipItState.plist references while the
    // second download is still in flight, a subsequent quitAndInstall() will
    // fail with ENOENT and the app will never relaunch.
    updaterIt(
      'should install the staged update when quitAndInstall is called while a second check is in flight',
      async (ctx) => {
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-race',
            endFixture: 'update-race'
          },
          async (appPath, updateZipPath) => {
            let downloadCount = 0;
            let stalledResponse: RoutedResponse | null = null;

            ctx.server.get('/update-file', (req, res) => {
              downloadCount++;
              if (downloadCount === 1) {
                // First download completes normally and stages the update.
                res.download(updateZipPath);
              } else {
                // Second download: stall indefinitely to simulate a slow
                // network. This keeps the second check "in progress" when
                // quitAndInstall() fires. Hold onto the response so we can
                // clean it up later.
                stalledResponse = res;
              }
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();

            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(launchResult.out).to.include('Calling quitAndInstall mid-download');
              // First check + first download + second check + stalled second download.
              expect(ctx.requests).to.have.lengthOf(4);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[2]).to.have.property('url', '/update-check');
              expect(ctx.requests[3]).to.have.property('url', '/update-file');
              // The second download must have been in flight (never completed)
              // when quitAndInstall was called.
              expect(launchResult.out).to.not.include('Unexpected second download completion');
            });

            // Unblock the stalled response now that the initial app has exited
            // so the server can shut down cleanly.
            if (stalledResponse) {
              (stalledResponse as RoutedResponse).status(500).end();
            }

            // The originally staged update (2.0.0) must have been applied and
            // the app must relaunch, proving the staged update directory was
            // not pruned out from under ShipItState.plist.
            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(5);
            expect(ctx.requests[4].url).to.equal('/update-check/updated/2.0.0');
            expect(ctx.requests[4].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );
  });
});
