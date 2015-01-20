var app = require('app');

function getDate() {
  var today = new Date();
  var year = today.getFullYear();
  var month = today.getMonth() + 1;
  if (month <= 9)
    month = '0' + month;
  var day= today.getDate();
  if (day <= 9)
    day = '0' + day;
  return year + '-' + month + '-' + day;
}

app.on('ready', function() {
  var json = {};
  json.version = process.versions['atom-shell'];
  json.date = getDate();

  var names = ['v8', 'uv', 'zlib', 'openssl', 'modules', 'chrome']
  for (var i in names) {
    var name = names[i];
    json[name] = process.versions[name];
  }

  json.files = [
    'darwin-x64',
    'darwin-x64-symbols',
    'linux-ia32',
    'linux-ia32-symbols',
    'linux-x64',
    'linux-x64-symbols',
    'win32-ia32',
    'win32-ia32-symbols',
  ];

  console.log(json);
  process.exit(0);
});
