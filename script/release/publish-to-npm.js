const temp = require('temp');
const fs = require('fs');
const path = require('path');
const childProcess = require('child_process');
const got = require('got');
const semver = require('semver');

const { getCurrentBranch, ELECTRON_DIR } = require('../lib/utils');
const rootPackageJson = require('../../package.json');

const { Octokit } = require('@octokit/rest');
const { getAssetContents } = require('./get-asset');
const octokit = new Octokit({
  userAgent: 'electron-npm-publisher',
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

if (!process.env.ELECTRON_NPM_OTP) {
  console.error('Please set ELECTRON_NPM_OTP');
  process.exit(1);
}

let tempDir;
temp.track(); // track and cleanup files at exit

const files = [
  'cli.js',
  'index.js',
  'install.js',
  'package.json',
  'versions.json',
  'README.md',
  'LICENSE'
];

const jsonFields = [
  'name',
  'version',
  'repository',
  'description',
  'license',
  'author',
  'keywords'
];

let npmTag = '';

new Promise((resolve, reject) => {
  temp.mkdir('electron-npm', (err, dirPath) => {
    if (err) {
      reject(err);
    } else {
      resolve(dirPath);
    }
  });
})
  .then((dirPath) => {
    tempDir = dirPath;
    // copy files from `/npm` to temp directory
    files.forEach((name) => {
      const noThirdSegment = name === 'README.md' || name === 'LICENSE';
      fs.writeFileSync(
        path.join(tempDir, name),
        fs.readFileSync(path.join(ELECTRON_DIR, noThirdSegment ? '' : 'npm', name))
      );
    });
    // copy from root package.json to temp/package.json
    const packageJson = require(path.join(tempDir, 'package.json'));
    jsonFields.forEach((fieldName) => {
      packageJson[fieldName] = rootPackageJson[fieldName];
    });
    fs.writeFileSync(
      path.join(tempDir, 'package.json'),
      JSON.stringify(packageJson, null, 2)
    );

    return octokit.repos.listReleases({
      owner: 'electron',
      repo: rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron'
    });
  })
  .then((releases) => {
  // download electron.d.ts from release
    const release = releases.data.find(
      (release) => release.tag_name === `v${rootPackageJson.version}`
    );
    if (!release) {
      throw new Error(`cannot find release with tag v${rootPackageJson.version}`);
    }
    return release;
  })
  .then(async (release) => {
    const tsdAsset = release.assets.find((asset) => asset.name === 'electron.d.ts');
    if (!tsdAsset) {
      throw new Error(`cannot find electron.d.ts from v${rootPackageJson.version} release assets`);
    }

    const typingsContent = await getAssetContents(
      rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron',
      tsdAsset.id
    );

    fs.writeFileSync(path.join(tempDir, 'electron.d.ts'), typingsContent);

    return release;
  })
  .then(async (release) => {
    const checksumsAsset = release.assets.find((asset) => asset.name === 'SHASUMS256.txt');
    if (!checksumsAsset) {
      throw new Error(`cannot find SHASUMS256.txt from v${rootPackageJson.version} release assets`);
    }

    const checksumsContent = await getAssetContents(
      rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron',
      checksumsAsset.id
    );

    const checksumsObject = {};
    for (const line of checksumsContent.trim().split('\n')) {
      const [checksum, file] = line.split(' *');
      checksumsObject[file] = checksum;
    }

    fs.writeFileSync(path.join(tempDir, 'checksums.json'), JSON.stringify(checksumsObject, null, 2));

    return release;
  })
  .then(async (release) => {
    const currentBranch = await getCurrentBranch();

    if (release.tag_name.indexOf('nightly') > 0) {
      // TODO(main-migration): Simplify once main branch is renamed.
      if (currentBranch === 'master' || currentBranch === 'main') {
        // Nightlies get published to their own module, so they should be tagged as latest
        npmTag = 'latest';
      } else {
        npmTag = `nightly-${currentBranch}`;
      }

      const currentJson = JSON.parse(fs.readFileSync(path.join(tempDir, 'package.json'), 'utf8'));
      currentJson.name = 'electron-nightly';
      rootPackageJson.name = 'electron-nightly';

      fs.writeFileSync(
        path.join(tempDir, 'package.json'),
        JSON.stringify(currentJson, null, 2)
      );
    } else {
      if (currentBranch === 'master' || currentBranch === 'main') {
        // This should never happen, main releases should be nightly releases
        // this is here just-in-case
        throw new Error('Unreachable release phase, can\'t tag a non-nightly release on the main branch');
      } else if (!release.prerelease) {
        // Tag the release with a `2-0-x` style tag
        npmTag = currentBranch;
      } else if (release.tag_name.indexOf('alpha') > 0) {
        // Tag the release with an `alpha-3-0-x` style tag
        npmTag = `alpha-${currentBranch}`;
      } else {
        // Tag the release with a `beta-3-0-x` style tag
        npmTag = `beta-${currentBranch}`;
      }
    }
  })
  .then(() => {
    const DEPSContent = fs.readFileSync(path.resolve(__dirname, '..', '..', 'DEPS'), 'utf8');

    const getVersion = (dep) => {
      // e.g: /'chromium_version':(\W+)(.+?)'/
      const re = new RegExp(`'${dep}':(\\W+)(.+?)'`);
      return re.exec(DEPSContent)[2];
    };

    const chromiumVersion = getVersion('chromium_version');
    // v16.0.0 => 16.0.0
    const nodeVersion = getVersion('node_version').slice(1);

    fs.writeFileSync(path.join(tempDir, 'versions.json'), JSON.stringify({
      chromium: chromiumVersion,
      node: nodeVersion,
      chromiumShort: chromiumVersion.split('.')[0],
      nodeShort: nodeVersion.split('.')[0]
    }, null, 2));
  })
  .then(() => childProcess.execSync('npm pack', { cwd: tempDir }))
  .then(() => {
  // test that the package can install electron prebuilt from github release
    const tarballPath = path.join(tempDir, `${rootPackageJson.name}-${rootPackageJson.version}.tgz`);
    return new Promise((resolve, reject) => {
      const result = childProcess.spawnSync('npm', ['install', tarballPath, '--force', '--silent'], {
        env: Object.assign({}, process.env, { electron_config_cache: tempDir }),
        cwd: tempDir,
        stdio: 'inherit'
      });
      if (result.status !== 0) {
        return reject(new Error(`npm install failed with status ${result.status}`));
      }
      try {
        const electronPath = require(path.resolve(tempDir, 'node_modules', rootPackageJson.name));
        if (typeof electronPath !== 'string') {
          return reject(new Error(`path to electron binary (${electronPath}) returned by the ${rootPackageJson.name} module is not a string`));
        }
        if (!fs.existsSync(electronPath)) {
          return reject(new Error(`path to electron binary (${electronPath}) returned by the ${rootPackageJson.name} module does not exist on disk`));
        }
      } catch (e) {
        console.error(e);
        return reject(new Error(`loading the generated ${rootPackageJson.name} module failed with an error`));
      }
      resolve(tarballPath);
    });
  })
  .then((tarballPath) => {
    const existingVersionJSON = childProcess.execSync(`npm view electron@${rootPackageJson.version} --json`).toString('utf-8');
    // It's possible this is a re-run and we already have published the package, if not we just publish like normal
    if (!existingVersionJSON) {
      childProcess.execSync(`npm publish ${tarballPath} --tag ${npmTag} --otp=${process.env.ELECTRON_NPM_OTP}`);
    }
  })
  .then(() => {
    const currentTags = JSON.parse(childProcess.execSync('npm show electron dist-tags --json').toString());
    const localVersion = rootPackageJson.version;
    const parsedLocalVersion = semver.parse(localVersion);
    if (rootPackageJson.name === 'electron') {
      // We should only customly add dist tags for non-nightly releases where the package name is still
      // "electron"
      if (parsedLocalVersion.prerelease.length === 0 &&
            semver.gt(localVersion, currentTags.latest)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} latest --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
      if (parsedLocalVersion.prerelease[0] === 'beta' &&
            semver.gt(localVersion, currentTags.beta)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} beta --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
      if (parsedLocalVersion.prerelease[0] === 'alpha' &&
            semver.gt(localVersion, currentTags.alpha)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} alpha --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
    }
  })
  .catch((err) => {
    console.error('Error:', err);
    process.exit(1);
  });
