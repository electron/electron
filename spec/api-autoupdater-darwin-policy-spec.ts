import { autoUpdater } from 'electron';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import { randomUUID } from 'node:crypto';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';

import { Mutation, setupUpdaterHarness, shouldRunUpdaterSpecs } from './lib/autoupdater-darwin-helpers';
import { ifdescribe } from './lib/spec-helpers';

// When Squirrel.Mac refuses or alters an update: version rules, a running
// app, tampered payloads, JSON update mode and direct contents writes. The
// update lifecycle itself is in api-autoupdater-darwin-spec.ts; the two files
// share a harness and are split so that they can land on different CI shards.
ifdescribe(shouldRunUpdaterSpecs)('autoUpdater behavior', function () {
  this.timeout(120000);

  const { logOnError, updaterIt } = setupUpdaterHarness();

  describe('with update server', () => {
    updaterIt('should update to lower version numbers', async (ctx) => {
      await ctx.withUpdatableApp(
        {
          nextVersion: '0.0.1',
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
          expect(ctx.requests[2].url).to.equal('/update-check/updated/0.0.1');
          expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
        }
      );
    });

    updaterIt('should abort the update if the application is still running when ShipIt kicks off', async (ctx) => {
      await ctx.withUpdatableApp(
        {
          nextVersion: '2.0.0',
          startFixture: 'update',
          endFixture: 'update'
        },
        async (appPath, updateZipPath) => {
          ctx.serveUpdate(updateZipPath);

          enum FlipFlop {
            INITIAL,
            FLIPPED,
            FLOPPED
          }

          // ShipIt should appear, find the retainer still running, and quit
          // without installing. Poll in a loop rather than on an interval, so
          // a slow `ps` cannot pile up behind itself.
          const shipItFlipFlopPromise = (async () => {
            let state = FlipFlop.INITIAL;
            while (state !== FlipFlop.FLOPPED) {
              if (ctx.signal.aborted) throw ctx.signal.reason;
              const running = await ctx.getRunningShipIts(appPath);
              switch (state) {
                case FlipFlop.INITIAL: {
                  if (running.length) state = FlipFlop.FLIPPED;
                  break;
                }
                case FlipFlop.FLIPPED: {
                  if (!running.length) state = FlipFlop.FLOPPED;
                  break;
                }
              }
              if (state !== FlipFlop.FLOPPED) await delay(500);
            }
          })();

          const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
          const retainerHandle = ctx.spawnAppWithHandle(appPath, ['remain-open']);
          try {
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await shipItFlipFlopPromise;
            expect(ctx.requests).to.have.lengthOf(2, 'should not have relaunched the updated app');
            expect(
              JSON.parse(
                await fs.promises.readFile(path.resolve(appPath, 'Contents/Resources/app/package.json'), 'utf8')
              ).version
            ).to.equal('1.0.0', 'should still be the old version on disk');
          } finally {
            retainerHandle.kill('SIGINT');
          }
        }
      );
    });

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the zip signature is invalid',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'add-resource',
              mutate: async (appPath) => {
                const resourcesPath = path.resolve(appPath, 'Contents', 'Resources', 'app', 'injected.txt');
                await fs.promises.writeFile(resourcesPath, 'demo');
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include('a sealed resource is missing or invalid');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the ShipIt binary is a symlink',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'modify-shipit',
              mutate: async (appPath) => {
                const shipItPath = path.resolve(
                  appPath,
                  'Contents',
                  'Frameworks',
                  'Squirrel.framework',
                  'Resources',
                  'ShipIt'
                );
                await fs.promises.rm(shipItPath, { force: true, recursive: true });
                await fs.promises.symlink('/tmp/ShipIt', shipItPath, 'file');
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include('a sealed resource is missing or invalid');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the Electron Framework is modified',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'modify-eframework',
              mutate: async (appPath) => {
                const shipItPath = path.resolve(
                  appPath,
                  'Contents',
                  'Frameworks',
                  'Electron Framework.framework',
                  'Electron Framework'
                );
                await fs.promises.appendFile(shipItPath, Buffer.from('123'));
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include(' main executable failed strict validation');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the zip extraction process fails to launch',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update'
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchAppSandboxed(
              appPath,
              path.resolve(__dirname, 'fixtures/auto-update/sandbox/block-ditto.sb'),
              [`http://localhost:${ctx.port}/update-check`]
            );
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Starting ditto task failed with error:');
              expect(launchResult.out).to.include('SQRLZipArchiverErrorDomain');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided with JSON update mode',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-json',
            endFixture: 'update-json'
          },
          async (appPath, updateZipPath) => {
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateZipPath);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                currentRelease: '2.0.0',
                releases: [
                  {
                    version: '2.0.0',
                    updateTo: {
                      version: '2.0.0',
                      url: `http://localhost:${ctx.port}/update-file`,
                      name: 'My Release Name',
                      notes: 'Theses are some release notes innit',
                      pub_date: new Date().toString()
                    }
                  }
                ]
              });
            });
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
            expect(ctx.requests[2]).to.have.property('url', '/update-check/updated/2.0.0');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and not update in JSON update mode when the currentRelease is older than the current version',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '0.1.0',
            startFixture: 'update-json',
            endFixture: 'update-json'
          },
          async (appPath, updateZipPath) => {
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateZipPath);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                currentRelease: '0.1.0',
                releases: [
                  {
                    version: '0.1.0',
                    updateTo: {
                      version: '0.1.0',
                      url: `http://localhost:${ctx.port}/update-file`,
                      name: 'My Release Name',
                      notes: 'Theses are some release notes innit',
                      pub_date: new Date().toString()
                    }
                  }
                ]
              });
            });
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('No update available');
              expect(ctx.requests).to.have.lengthOf(1);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    // Nested describes go last; see the note at the top of this block.

    describe('with ElectronSquirrelPreventDowngrades enabled', () => {
      const preventDowngrades: Mutation = {
        mutationKey: 'prevent-downgrades',
        mutate: async (appPath) => {
          const infoPath = path.resolve(appPath, 'Contents', 'Info.plist');
          await fs.promises.writeFile(
            infoPath,
            (await fs.promises.readFile(infoPath, 'utf8')).replace(
              '<key>NSSupportsAutomaticGraphicsSwitching</key>',
              '<key>ElectronSquirrelPreventDowngrades</key><true/><key>NSSupportsAutomaticGraphicsSwitching</key>'
            )
          );
        }
      };

      updaterIt('should not update to lower version numbers', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '0.0.1',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPreSign: preventDowngrades
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Cannot update to a bundle with a lower version number');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      });

      updaterIt('should not update to version strings that are not simple Major.Minor.Patch', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0-bad',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPreSign: preventDowngrades
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Cannot update to a bundle with a lower version number');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      });

      updaterIt('should still update to higher version numbers', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '1.0.1',
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
            expect(ctx.requests[2].url).to.equal('/update-check/updated/1.0.1');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      });

      it('should compare version numbers correctly', () => {
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.0', '2.0.0')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.1', '1.0.10')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.10', '1.0.1')).to.equal(false);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.31.1', '1.32.0')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.31.1', '0.32.0')).to.equal(false);
      });
    });

    describe('with SquirrelMacEnableDirectContentsWrite enabled', () => {
      updaterIt(
        'should hit the download endpoint when an update is available and update successfully when the zip is provided leaving the parent directory untouched',
        async (ctx) => {
          ctx.setUserDefault('SquirrelMacEnableDirectContentsWrite', true);
          try {
            await ctx.withUpdatableApp(
              {
                nextVersion: '2.0.0',
                startFixture: 'update',
                endFixture: 'update'
              },
              async (appPath, updateZipPath) => {
                const randomID = randomUUID();
                cp.spawnSync('xattr', ['-w', 'spec-id', randomID, appPath]);
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
                const result = cp.spawnSync('xattr', ['-l', appPath]);
                expect(result.stdout.toString()).to.include(`spec-id: ${randomID}`);
              }
            );
          } finally {
            ctx.setUserDefault('SquirrelMacEnableDirectContentsWrite', null);
          }
        }
      );
    });
  });
});
