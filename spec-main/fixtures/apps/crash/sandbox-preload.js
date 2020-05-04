const { crashReporter } = require('electron');
crashReporter.start({
  productName: 'Zombies',
  companyName: 'Umbrella Corporation',
  uploadToServer: true,
  ignoreSystemCrashHandler: true,
  submitURL: '',
  extra: {
    'extra1': 'extra1',
    'extra2': 'extra2'
  }
});
const params = new URLSearchParams(location.search);
if (params.get('set_extra') === '1') {
  crashReporter.addExtraParameter('extra3', 'added');
  crashReporter.removeExtraParameter('extra1');
}
process.crash();
