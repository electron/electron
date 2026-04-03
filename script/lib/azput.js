/* eslint-disable camelcase */

const { BlobServiceClient, ContainerClient } = require('@azure/storage-blob');
const minimist = require('minimist');

const path = require('node:path');

const { ELECTRON_ARTIFACTS_BLOB_STORAGE } = process.env;
if (!ELECTRON_ARTIFACTS_BLOB_STORAGE) {
  console.error('Missing required ELECTRON_ARTIFACTS_BLOB_STORAGE environment variable.');
  process.exit(1);
}

let getContainerClient;
if (ELECTRON_ARTIFACTS_BLOB_STORAGE.startsWith('{')) {
  const containerUrls = JSON.parse(ELECTRON_ARTIFACTS_BLOB_STORAGE);
  getContainerClient = (name) => {
    if (!containerUrls[name]) throw new Error(`No SAS URL provided for container '${name}'`);
    return new ContainerClient(containerUrls[name]);
  };
} else {
  const blobServiceClient = ELECTRON_ARTIFACTS_BLOB_STORAGE.startsWith('https://')
    ? new BlobServiceClient(ELECTRON_ARTIFACTS_BLOB_STORAGE)
    : BlobServiceClient.fromConnectionString(ELECTRON_ARTIFACTS_BLOB_STORAGE);
  getContainerClient = (name) => blobServiceClient.getContainerClient(name);
}

const args = minimist(process.argv.slice(2));

let { prefix = '/', key_prefix = '', _: files } = args;
if (prefix && !prefix.endsWith(path.sep)) prefix = path.resolve(prefix) + path.sep;

function filenameToKey (file) {
  file = path.resolve(file);
  if (file.startsWith(prefix)) file = file.substr(prefix.length - 1);
  return key_prefix + (path.sep === '\\' ? file.replace(/\\/g, '/') : file);
}

let anErrorOccurred = false;
function next (done) {
  const file = files.shift();
  if (!file) return done();
  const key = filenameToKey(file);

  const [containerName, ...keyPath] = key.split('/');
  const blobKey = keyPath.join('/');
  console.log(`Uploading '${file}' to container '${containerName}' with key '${blobKey}'...`);

  const containerClient = getContainerClient(containerName);
  const blockBlobClient = containerClient.getBlockBlobClient(blobKey);
  blockBlobClient.uploadFile(file)
    .then((uploadBlobResponse) => {
      console.log(`Upload block blob ${blobKey} successfully: https://artifacts.electronjs.org/${key}`, uploadBlobResponse.requestId);
    })
    .catch((err) => {
      console.error(err);
      anErrorOccurred = true;
    })
    .then(() => next(done));
}
next(() => {
  process.exit(anErrorOccurred ? 1 : 0);
});
