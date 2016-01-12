{deprecate} = require 'electron'
{webFrame} = process.atomBinding 'web_frame'

### Deprecated. ###
deprecate.rename webFrame, 'registerUrlSchemeAsSecure', 'registerURLSchemeAsSecure'
deprecate.rename webFrame, 'registerUrlSchemeAsBypassingCSP', 'registerURLSchemeAsBypassingCSP'
deprecate.rename webFrame, 'registerUrlSchemeAsPrivileged', 'registerURLSchemeAsPrivileged'

module.exports = webFrame
