const { fromPartition, fromPath } = process._linkedBinding('electron_browser_session');

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
