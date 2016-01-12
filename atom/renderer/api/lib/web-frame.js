var deprecate, webFrame;

deprecate = require('electron').deprecate;

webFrame = process.atomBinding('web_frame').webFrame;


/* Deprecated. */

deprecate.rename(webFrame, 'registerUrlSchemeAsSecure', 'registerURLSchemeAsSecure');

deprecate.rename(webFrame, 'registerUrlSchemeAsBypassingCSP', 'registerURLSchemeAsBypassingCSP');

deprecate.rename(webFrame, 'registerUrlSchemeAsPrivileged', 'registerURLSchemeAsPrivileged');

module.exports = webFrame;
