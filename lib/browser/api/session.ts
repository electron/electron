const { fromPartition } = process.electronBinding('session');

const sessionAPI = {
  fromPartition,
  get defaultSession () {
    return fromPartition('');
  }
};

export default sessionAPI;
