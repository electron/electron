const fs = require('fs')
const https = require('https')
const path = require('path')

const server = https.createServer({
  key: fs.readFileSync(path.resolve(__dirname, 'tls.key.pem')),
  cert: fs.readFileSync(path.resolve(__dirname, 'tls.cert.pem'))
}, (req, res) => {
  res.end(JSON.stringify({ protocol: req.socket.getProtocol() }))

  setTimeout(() => {
    server.close()
  }, 0)
})

server.listen(0, () => {
  console.log(server.address().port)
})
