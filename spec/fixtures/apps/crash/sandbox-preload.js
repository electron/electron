const { crashReporter } = require('electron');

const params = new URLSearchParams(location.search);
if (params.get('set_extra') === '1') {
  crashReporter.addExtraParameter('rendererSpecific', 'rs');
  crashReporter.addExtraParameter('addedThenRemoved', 'to-be-removed');
  crashReporter.removeExtraParameter('addedThenRemoved');
}

process.crash();
