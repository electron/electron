/* eslint-disable camelcase */
const AWS = require('aws-sdk');
const fs = require('fs');
const path = require('path');
AWS.config.update({ region: 'us-west-2' });
const s3 = new AWS.S3({ apiVersion: '2006-03-01' });

const args = require('minimist')(process.argv.slice(2));

let { bucket, prefix = '/', key_prefix = '', grant, _: files } = args;
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
  console.log(`Uploading '${file}' to bucket '${bucket}' with key '${key}'...`);
  s3.upload({
    Bucket: bucket,
    Key: key,
    Body: fs.createReadStream(file),
    ACL: grant
  }, (err, data) => {
    if (err) {
      console.error(err);
      anErrorOccurred = true;
    }
    next(done);
  });
}
next(() => {
  process.exit(anErrorOccurred ? 1 : 0);
});
