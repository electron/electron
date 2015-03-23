var fs = require('fs');
var path = require('path');
var runas = require('runas');

/* FIXME: C:\\Program Files\\ should comes from env variable
 And for 32 bit msdia80 is should be placed at %ProgramFiles(x86)% */
var source = path.resolve(__dirname, '..', '..', 'vendor', 'breakpad', 'msdia80.dll');
var target = 'C:\\Program Files\\Common Files\\Microsoft Shared\\VC\\msdia80.dll';
if (fs.existsSync(target))
  return;

runas('cmd', ['/K',
        'copy', source, target,
        '&', 'regsvr32', '/s', target,
        '&', 'exit'],
        {admin:true}
);