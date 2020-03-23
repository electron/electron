'use strict';

// Common modules, please sort alphabetically
module.exports = [
  { name: 'clipboard', loader: () => require('./clipboard') },
  { name: 'nativeImage', loader: () => require('./native-image') },
  { name: 'shell', loader: () => require('./shell') },
  // The internal modules, invisible unless you know their names.
  { name: 'deprecate', loader: () => require('./deprecate'), private: true }
];
