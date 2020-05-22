const { fromPartition } = process.electronBinding('session');

export default {
  fromPartition,
  get defaultSession () {
    return fromPartition('');
  }
};
