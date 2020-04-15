'use strict';

// parses a feature string that has the format used in window.open()
// - `features` input string
// - `emit` function(key, value) - called for each parsed KV
function parseFeaturesString (features, emit) {
  features = `${features}`.trim();
  // split the string by ','
  features.split(/\s*,\s*/).forEach((feature) => {
    // expected form is either a key by itself or a key/value pair in the form of
    // 'key=value'
    let [key, value] = feature.split(/\s*=\s*/);
    if (!key) return;

    // interpret the value as a boolean, if possible
    value = (value === 'yes' || value === '1') ? true : (value === 'no' || value === '0') ? false : value;

    // emit the parsed pair
    emit(key, value);
  });
}

function convertFeaturesString (features, frameName) {
  const options = {};

  const ints = ['x', 'y', 'width', 'height', 'minWidth', 'maxWidth', 'minHeight', 'maxHeight', 'zoomFactor'];
  const webPreferences = ['zoomFactor', 'nodeIntegration', 'enableRemoteModule', 'preload', 'javascript', 'contextIsolation', 'webviewTag'];

  // Used to store additional features
  const additionalFeatures = [];

  // Parse the features
  parseFeaturesString(features, function (key, value) {
    if (value === undefined) {
      additionalFeatures.push(key);
    } else {
      // Don't allow webPreferences to be set since it must be an object
      // that cannot be directly overridden
      if (key === 'webPreferences') return;

      if (webPreferences.includes(key)) {
        if (options.webPreferences == null) {
          options.webPreferences = {};
        }
        options.webPreferences[key] = value;
      } else {
        options[key] = value;
      }
    }
  });

  if (options.left) {
    if (options.x == null) {
      options.x = options.left;
    }
  }
  if (options.top) {
    if (options.y == null) {
      options.y = options.top;
    }
  }
  if (options.title == null) {
    options.title = frameName;
  }
  if (options.width == null) {
    options.width = 800;
  }
  if (options.height == null) {
    options.height = 600;
  }

  for (const name of ints) {
    if (options[name] != null) {
      options[name] = parseInt(options[name], 10);
    }
  }

  return {
    options, additionalFeatures
  };
}

module.exports = {
  parseFeaturesString, convertFeaturesString
};
