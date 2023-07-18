const { spawnSync } = require('node:child_process');
const path = require('node:path');

module.exports.getElectronVersion = () => {
  // Find the nearest tag to the current HEAD
  // This is equivilant to our old logic of "use a value in package.json" for the following reasons
  //
  // 1. Whenever we updated the package.json we ALSO pushed a tag with the same version
  // 2. Whenever we _reverted_ a bump all we actually did was push a commit that deleted the tag and changed the version number back
  //
  // The only difference in the "git describe" technique is that technically a commit can "change" it's version
  // number if a tag is created / removed retroactively.  i.e. the first time a commit is pushed it will be 1.2.3
  // and after the tag is made rebuilding the same commit will result in it being 1.2.4
  const output = spawnSync('git', ['describe', '--tags', '--abbrev=0'], {
    cwd: path.resolve(__dirname, '..', '..')
  });
  if (output.status !== 0) {
    console.error(output.stderr);
    throw new Error('Failed to get current electron version');
  }
  return output.stdout.toString().trim().replace(/^v/g, '');
};
