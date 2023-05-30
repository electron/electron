const net = require('node:net');
const server = net.createServer(function () {});
server.listen(process.argv[2]);
process.exit(0);
