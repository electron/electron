#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load();

const args = require('minimist')(process.argv.slice(2), {
  boolean: [
    'validateRelease',
    'skipVersionCheck',
    'automaticRelease',
    'verboseNugget'
  ],
  default: { verboseNugget: false }
});
const fs = require('fs');
const { execSync } = require('child_process');
const nugget = require('nugget');
const got = require('got');
const pkg = require('../../package.json');
const pkgVersion = `v${pkg.version}`;
const path = require('path');
const sumchecker = require('sumchecker');
const temp = require('temp').track();
const { URL } = require('url');
const { Octokit } = require('@octokit/rest');
const AWS = require('aws-sdk');

require('colors');
const pass = '✓'.green;
const fail = '✗'.red;

const { ELECTRON_DIR } = require('../lib/utils');

const octokit = new Octokit({
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

const targetRepo = pkgVersion.indexOf('nightly') > 0 ? 'nightlies' : 'electron';
let failureCount = 0;

async function getDraftRelease (version, skipValidation) {
  const releaseInfo = await octokit.repos.listReleases({
    owner: 'electron',
    repo: targetRepo
  });

  const versionToCheck = version || pkgVersion;
  const drafts = releaseInfo.data.filter(release => {
    return release.tag_name === versionToCheck && release.draft === true;
  });

  const draft = drafts[0];
  if (!skipValidation) {
    failureCount = 0;
    check(drafts.length === 1, 'one draft exists', true);
    if (versionToCheck.indexOf('beta') > -1) {
      check(draft.prerelease, 'draft is a prerelease');
    }
    check(draft.body.length > 50 && !draft.body.includes('(placeholder)'), 'draft has release notes');
    check((failureCount === 0), 'Draft release looks good to go.', true);
  }
  return draft;
}

async function validateReleaseAssets (release, validatingRelease) {
  const requiredAssets = assetsForVersion(release.tag_name, validatingRelease).sort();
  const extantAssets = release.assets.map(asset => asset.name).sort();
  const downloadUrls = release.assets.map(asset => asset.browser_download_url).sort();

  failureCount = 0;
  requiredAssets.forEach(asset => {
    check(extantAssets.includes(asset), asset);
  });
  check((failureCount === 0), 'All required GitHub assets exist for release', true);

  if (!validatingRelease || !release.draft) {
    if (release.draft) {
      await verifyAssets(release);
    } else {
      await verifyShasums(downloadUrls)
        .catch(err => {
          console.log(`${fail} error verifyingShasums`, err);
        });
    }
    const s3Urls = s3UrlsForVersion(release.tag_name);
    await verifyShasums(s3Urls, true);
  }
}

function check (condition, statement, exitIfFail = false) {
  if (condition) {
    console.log(`${pass} ${statement}`);
  } else {
    failureCount++;
    console.log(`${fail} ${statement}`);
    if (exitIfFail) process.exit(1);
  }
}

function assetsForVersion (version, validatingRelease) {
  const patterns = [
    `chromedriver-${version}-darwin-x64.zip`,
    `chromedriver-${version}-darwin-arm64.zip`,
    `chromedriver-${version}-linux-arm64.zip`,
    `chromedriver-${version}-linux-armv7l.zip`,
    `chromedriver-${version}-linux-ia32.zip`,
    `chromedriver-${version}-linux-x64.zip`,
    `chromedriver-${version}-mas-x64.zip`,
    `chromedriver-${version}-mas-arm64.zip`,
    `chromedriver-${version}-win32-ia32.zip`,
    `chromedriver-${version}-win32-x64.zip`,
    `chromedriver-${version}-win32-arm64.zip`,
    `electron-${version}-darwin-x64-dsym.zip`,
    `electron-${version}-darwin-x64-symbols.zip`,
    `electron-${version}-darwin-x64.zip`,
    `electron-${version}-darwin-arm64-dsym.zip`,
    `electron-${version}-darwin-arm64-symbols.zip`,
    `electron-${version}-darwin-arm64.zip`,
    `electron-${version}-linux-arm64-symbols.zip`,
    `electron-${version}-linux-arm64.zip`,
    `electron-${version}-linux-armv7l-symbols.zip`,
    `electron-${version}-linux-armv7l.zip`,
    `electron-${version}-linux-ia32-symbols.zip`,
    `electron-${version}-linux-ia32.zip`,
    `electron-${version}-linux-x64-debug.zip`,
    `electron-${version}-linux-x64-symbols.zip`,
    `electron-${version}-linux-x64.zip`,
    `electron-${version}-mas-x64-dsym.zip`,
    `electron-${version}-mas-x64-symbols.zip`,
    `electron-${version}-mas-x64.zip`,
    `electron-${version}-mas-arm64-dsym.zip`,
    `electron-${version}-mas-arm64-symbols.zip`,
    `electron-${version}-mas-arm64.zip`,
    `electron-${version}-win32-ia32-pdb.zip`,
    `electron-${version}-win32-ia32-symbols.zip`,
    `electron-${version}-win32-ia32.zip`,
    `electron-${version}-win32-x64-pdb.zip`,
    `electron-${version}-win32-x64-symbols.zip`,
    `electron-${version}-win32-x64.zip`,
    `electron-${version}-win32-arm64-pdb.zip`,
    `electron-${version}-win32-arm64-symbols.zip`,
    `electron-${version}-win32-arm64.zip`,
    'electron-api.json',
    'electron.d.ts',
    'hunspell_dictionaries.zip',
    `ffmpeg-${version}-darwin-x64.zip`,
    `ffmpeg-${version}-darwin-arm64.zip`,
    `ffmpeg-${version}-linux-arm64.zip`,
    `ffmpeg-${version}-linux-armv7l.zip`,
    `ffmpeg-${version}-linux-ia32.zip`,
    `ffmpeg-${version}-linux-x64.zip`,
    `ffmpeg-${version}-mas-x64.zip`,
    `ffmpeg-${version}-mas-arm64.zip`,
    `ffmpeg-${version}-win32-ia32.zip`,
    `ffmpeg-${version}-win32-x64.zip`,
    `ffmpeg-${version}-win32-arm64.zip`,
    `mksnapshot-${version}-darwin-x64.zip`,
    `mksnapshot-${version}-darwin-arm64.zip`,
    `mksnapshot-${version}-linux-arm64-x64.zip`,
    `mksnapshot-${version}-linux-armv7l-x64.zip`,
    `mksnapshot-${version}-linux-ia32.zip`,
    `mksnapshot-${version}-linux-x64.zip`,
    `mksnapshot-${version}-mas-x64.zip`,
    `mksnapshot-${version}-mas-arm64.zip`,
    `mksnapshot-${version}-win32-ia32.zip`,
    `mksnapshot-${version}-win32-x64.zip`,
    `mksnapshot-${version}-win32-arm64-x64.zip`,
    `electron-${version}-win32-ia32-toolchain-profile.zip`,
    `electron-${version}-win32-x64-toolchain-profile.zip`,
    `electron-${version}-win32-arm64-toolchain-profile.zip`
  ];
  if (!validatingRelease) {
    patterns.push('SHASUMS256.txt');
  }
  return patterns;
}

function s3UrlsForVersion (version) {
  const bucket = 'https://gh-contractor-zcbenz.s3.amazonaws.com/';
  const patterns = [
    `${bucket}atom-shell/dist/${version}/iojs-${version}-headers.tar.gz`,
    `${bucket}atom-shell/dist/${version}/iojs-${version}.tar.gz`,
    `${bucket}atom-shell/dist/${version}/node-${version}.tar.gz`,
    `${bucket}atom-shell/dist/${version}/node.lib`,
    `${bucket}atom-shell/dist/${version}/win-x64/iojs.lib`,
    `${bucket}atom-shell/dist/${version}/win-x86/iojs.lib`,
    `${bucket}atom-shell/dist/${version}/x64/node.lib`,
    `${bucket}atom-shell/dist/${version}/SHASUMS.txt`,
    `${bucket}atom-shell/dist/${version}/SHASUMS256.txt`,
    `${bucket}atom-shell/dist/index.json`
  ];
  return patterns;
}

function runScript (scriptName, scriptArgs, cwd) {
  const scriptCommand = `${scriptName} ${scriptArgs.join(' ')}`;
  const scriptOptions = {
    encoding: 'UTF-8'
  };
  if (cwd) scriptOptions.cwd = cwd;
  try {
    return execSync(scriptCommand, scriptOptions);
  } catch (err) {
    console.log(`${fail} Error running ${scriptName}`, err);
    process.exit(1);
  }
}

function uploadNodeShasums () {
  console.log('Uploading Node SHASUMS file to S3.');
  const scriptPath = path.join(ELECTRON_DIR, 'script', 'release', 'uploaders', 'upload-node-checksums.py');
  runScript(scriptPath, ['-v', pkgVersion]);
  console.log(`${pass} Done uploading Node SHASUMS file to S3.`);
}

function uploadIndexJson () {
  console.log('Uploading index.json to S3.');
  const scriptPath = path.join(ELECTRON_DIR, 'script', 'release', 'uploaders', 'upload-index-json.py');
  runScript(scriptPath, [pkgVersion]);
  console.log(`${pass} Done uploading index.json to S3.`);
}

async function mergeShasums (pkgVersion) {
  // Download individual checksum files for Electron zip files from S3,
  // concatenate them, and upload to GitHub.

  const bucket = process.env.ELECTRON_S3_BUCKET;
  const accessKeyId = process.env.ELECTRON_S3_ACCESS_KEY;
  const secretAccessKey = process.env.ELECTRON_S3_SECRET_KEY;
  if (!bucket || !accessKeyId || !secretAccessKey) {
    throw new Error('Please set the $ELECTRON_S3_BUCKET, $ELECTRON_S3_ACCESS_KEY, and $ELECTRON_S3_SECRET_KEY environment variables');
  }

  const s3 = new AWS.S3({
    apiVersion: '2006-03-01',
    accessKeyId,
    secretAccessKey,
    region: 'us-west-2'
  });
  const objects = await s3.listObjectsV2({
    Bucket: bucket,
    Prefix: `atom-shell/tmp/${pkgVersion}/`,
    Delimiter: '/'
  }).promise();
  const shasums = [];
  for (const obj of objects.Contents) {
    if (obj.Key.endsWith('.sha256sum')) {
      const data = await s3.getObject({
        Bucket: bucket,
        Key: obj.Key
      }).promise();
      shasums.push(data.Body.toString('ascii').trim());
    }
  }
  return shasums.join('\n');
}

async function createReleaseShasums (release) {
  const fileName = 'SHASUMS256.txt';
  const existingAssets = release.assets.filter(asset => asset.name === fileName);
  if (existingAssets.length > 0) {
    console.log(`${fileName} already exists on GitHub; deleting before creating new file.`);
    await octokit.repos.deleteReleaseAsset({
      owner: 'electron',
      repo: targetRepo,
      asset_id: existingAssets[0].id
    }).catch(err => {
      console.log(`${fail} Error deleting ${fileName} on GitHub:`, err);
    });
  }
  console.log(`Creating and uploading the release ${fileName}.`);
  const checksums = await mergeShasums(pkgVersion);

  console.log(`${pass} Generated release SHASUMS.`);
  const filePath = await saveShaSumFile(checksums, fileName);

  console.log(`${pass} Created ${fileName} file.`);
  await uploadShasumFile(filePath, fileName, release.id);

  console.log(`${pass} Successfully uploaded ${fileName} to GitHub.`);
}

async function uploadShasumFile (filePath, fileName, releaseId) {
  const uploadUrl = `https://uploads.github.com/repos/electron/${targetRepo}/releases/${releaseId}/assets{?name,label}`;
  return octokit.repos.uploadReleaseAsset({
    url: uploadUrl,
    headers: {
      'content-type': 'text/plain',
      'content-length': fs.statSync(filePath).size
    },
    data: fs.createReadStream(filePath),
    name: fileName
  }).catch(err => {
    console.log(`${fail} Error uploading ${filePath} to GitHub:`, err);
    process.exit(1);
  });
}

function saveShaSumFile (checksums, fileName) {
  return new Promise((resolve, reject) => {
    temp.open(fileName, (err, info) => {
      if (err) {
        console.log(`${fail} Could not create ${fileName} file`);
        process.exit(1);
      } else {
        fs.writeFileSync(info.fd, checksums);
        fs.close(info.fd, (err) => {
          if (err) {
            console.log(`${fail} Could close ${fileName} file`);
            process.exit(1);
          }
          resolve(info.path);
        });
      }
    });
  });
}

async function publishRelease (release) {
  return octokit.repos.updateRelease({
    owner: 'electron',
    repo: targetRepo,
    release_id: release.id,
    tag_name: release.tag_name,
    draft: false
  }).catch(err => {
    console.log(`${fail} Error publishing release:`, err);
    process.exit(1);
  });
}

async function makeRelease (releaseToValidate) {
  if (releaseToValidate) {
    if (releaseToValidate === true) {
      releaseToValidate = pkgVersion;
    } else {
      console.log('Release to validate !=== true');
    }
    console.log(`Validating release ${releaseToValidate}`);
    const release = await getDraftRelease(releaseToValidate);
    await validateReleaseAssets(release, true);
  } else {
    let draftRelease = await getDraftRelease();
    uploadNodeShasums();
    uploadIndexJson();

    await createReleaseShasums(draftRelease);

    // Fetch latest version of release before verifying
    draftRelease = await getDraftRelease(pkgVersion, true);
    await validateReleaseAssets(draftRelease);
    await publishRelease(draftRelease);
    console.log(`${pass} SUCCESS!!! Release has been published. Please run ` +
      '"npm run publish-to-npm" to publish release to npm.');
  }
}

async function makeTempDir () {
  return new Promise((resolve, reject) => {
    temp.mkdir('electron-publish', (err, dirPath) => {
      if (err) {
        reject(err);
      } else {
        resolve(dirPath);
      }
    });
  });
}

async function verifyAssets (release) {
  const downloadDir = await makeTempDir();

  console.log('Downloading files from GitHub to verify shasums');
  const shaSumFile = 'SHASUMS256.txt';

  let filesToCheck = await Promise.all(release.assets.map(async asset => {
    const requestOptions = await octokit.repos.getReleaseAsset.endpoint({
      owner: 'electron',
      repo: targetRepo,
      asset_id: asset.id,
      headers: {
        Accept: 'application/octet-stream'
      }
    });

    const { url, headers } = requestOptions;
    headers.authorization = `token ${process.env.ELECTRON_GITHUB_TOKEN}`;

    const response = await got(url, {
      followRedirect: false,
      method: 'HEAD',
      headers
    });

    await downloadFiles(response.headers.location, downloadDir, asset.name);
    return asset.name;
  })).catch(err => {
    console.log(`${fail} Error downloading files from GitHub`, err);
    process.exit(1);
  });

  filesToCheck = filesToCheck.filter(fileName => fileName !== shaSumFile);
  let checkerOpts;
  await validateChecksums({
    algorithm: 'sha256',
    filesToCheck,
    fileDirectory: downloadDir,
    shaSumFile,
    checkerOpts,
    fileSource: 'GitHub'
  });
}

function downloadFiles (urls, directory, targetName) {
  return new Promise((resolve, reject) => {
    const nuggetOpts = { dir: directory };
    nuggetOpts.quiet = !args.verboseNugget;
    if (targetName) nuggetOpts.target = targetName;

    nugget(urls, nuggetOpts, (err) => {
      if (err) {
        reject(err);
      } else {
        console.log(`${pass} all files downloaded successfully!`);
        resolve();
      }
    });
  });
}

async function verifyShasums (urls, isS3) {
  const fileSource = isS3 ? 'S3' : 'GitHub';
  console.log(`Downloading files from ${fileSource} to verify shasums`);
  const downloadDir = await makeTempDir();
  let filesToCheck = [];
  try {
    if (!isS3) {
      await downloadFiles(urls, downloadDir);
      filesToCheck = urls.map(url => {
        const currentUrl = new URL(url);
        return path.basename(currentUrl.pathname);
      }).filter(file => file.indexOf('SHASUMS') === -1);
    } else {
      const s3VersionPath = `/atom-shell/dist/${pkgVersion}/`;
      await Promise.all(urls.map(async (url) => {
        const currentUrl = new URL(url);
        const dirname = path.dirname(currentUrl.pathname);
        const filename = path.basename(currentUrl.pathname);
        const s3VersionPathIdx = dirname.indexOf(s3VersionPath);
        if (s3VersionPathIdx === -1 || dirname === s3VersionPath) {
          if (s3VersionPathIdx !== -1 && filename.indexof('SHASUMS') === -1) {
            filesToCheck.push(filename);
          }
          await downloadFiles(url, downloadDir);
        } else {
          const subDirectory = dirname.substr(s3VersionPathIdx + s3VersionPath.length);
          const fileDirectory = path.join(downloadDir, subDirectory);
          try {
            fs.statSync(fileDirectory);
          } catch (err) {
            fs.mkdirSync(fileDirectory);
          }
          filesToCheck.push(path.join(subDirectory, filename));
          await downloadFiles(url, fileDirectory);
        }
      }));
    }
  } catch (err) {
    console.log(`${fail} Error downloading files from ${fileSource}`, err);
    process.exit(1);
  }
  console.log(`${pass} Successfully downloaded the files from ${fileSource}.`);
  let checkerOpts;
  if (isS3) {
    checkerOpts = { defaultTextEncoding: 'binary' };
  }

  await validateChecksums({
    algorithm: 'sha256',
    filesToCheck,
    fileDirectory: downloadDir,
    shaSumFile: 'SHASUMS256.txt',
    checkerOpts,
    fileSource
  });

  if (isS3) {
    await validateChecksums({
      algorithm: 'sha1',
      filesToCheck,
      fileDirectory: downloadDir,
      shaSumFile: 'SHASUMS.txt',
      checkerOpts,
      fileSource
    });
  }
}

async function validateChecksums (validationArgs) {
  console.log(`Validating checksums for files from ${validationArgs.fileSource} ` +
    `against ${validationArgs.shaSumFile}.`);
  const shaSumFilePath = path.join(validationArgs.fileDirectory, validationArgs.shaSumFile);
  const checker = new sumchecker.ChecksumValidator(validationArgs.algorithm,
    shaSumFilePath, validationArgs.checkerOpts);
  await checker.validate(validationArgs.fileDirectory, validationArgs.filesToCheck)
    .catch(err => {
      if (err instanceof sumchecker.ChecksumMismatchError) {
        console.error(`${fail} The checksum of ${err.filename} from ` +
          `${validationArgs.fileSource} did not match the shasum in ` +
          `${validationArgs.shaSumFile}`);
      } else if (err instanceof sumchecker.ChecksumParseError) {
        console.error(`${fail} The checksum file ${validationArgs.shaSumFile} ` +
          `from ${validationArgs.fileSource} could not be parsed.`, err);
      } else if (err instanceof sumchecker.NoChecksumFoundError) {
        console.error(`${fail} The file ${err.filename} from ` +
          `${validationArgs.fileSource} was not in the shasum file ` +
          `${validationArgs.shaSumFile}.`);
      } else {
        console.error(`${fail} Error matching files from ` +
          `${validationArgs.fileSource} shasums in ${validationArgs.shaSumFile}.`, err);
      }
      process.exit(1);
    });
  console.log(`${pass} All files from ${validationArgs.fileSource} match ` +
    `shasums defined in ${validationArgs.shaSumFile}.`);
}

makeRelease(args.validateRelease);
