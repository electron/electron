var fs = require('fs');
var path = require('path');
var runas = require('runas');

var source = path.resolve(__dirname, '..', '..', 'vendor', 'breakpad', 'msdia80.dll');
var target = 'C:\\Program Files\\Common Files\\Microsoft Shared\\VC\\msdia80.dll';
if (fs.existsSync(target))
  return;

runas('cmd',
      ['/K', 'copy', source, target, '&', 'regsvr32', '/s', target, '&', 'exit'],
      {admin: true});
