var os = require('os')
var path = require('path')
var pathExists = require('path-exists')
var mkdir = require('mkdirp')
var nugget = require('nugget')
var homePath = require('home-path')
var mv = require('mv')
var npmrc = require('rc')('npm')

module.exports = function download (opts, cb) {
  var platform = opts.platform || os.platform()
  var arch = opts.arch || os.arch()
  var version = opts.version
  var symbols = opts.symbols || false
  if (!version) return cb(new Error('must specify version'))
  var filename = 'tr-electron-v' + version + '-' + platform + '-' + arch + (symbols ? '-symbols' : '') + '.zip'
  var url = process.env.NPM_CONFIG_ELECTRON_MIRROR ||
    process.env.ELECTRON_MIRROR ||
    opts.mirror ||
    'https://bams-sami.int.thomsonreuters.com/artifactory/default.npm.global/UniversalContainer/tr-electron-releases'
  url += '/'
  url += process.env.ELECTRON_CUSTOM_FILENAME || opts.customFilename || filename
  
  var homeDir = homePath()
  
  var cache = opts.cache || path.join(homeDir, './.tr-electron')

  var strictSSL = true
  if (opts.strictSSL === false || npmrc['strict-ssl'] === false) {
    strictSSL = false
  }

  var proxy
  if (npmrc && npmrc.proxy) proxy = npmrc.proxy
  if (npmrc && npmrc['https-proxy']) proxy = npmrc['https-proxy']

  var cachedZip = path.join(cache, filename)
  if( pathExists.sync(cachedZip) ) {

      return cb(null, cachedZip)
    } else {

        mkCacheDir(function (err, actualCache) {
          if (err) {
              
              console.info('Failed to create cache dir = ' + actualCache) 
              return cb(err)
          }
          cachedZip = path.join(actualCache, filename) // in case cache dir changed
          // download to tmpdir
          var tmpdir = path.join(os.tmpdir(), 'electron-tmp-download-' + process.pid + '-' + Date.now())
          mkdir(tmpdir, function (err) {
            if (err) {

                console.info('Failed to create temp dir = ' + tmpdir) 
                return cb(err)
            }
            //debug('downloading zip', url, 'to', tmpdir)
            var nuggetOpts = {target: filename, dir: tmpdir, resume: true, verbose: true, strictSSL: strictSSL, proxy: proxy}
            nugget(url, nuggetOpts, function (errors) {
              if (errors) {
                var error = errors[0] // nugget returns an array of errors but we only need 1st because we only have 1 url
                if (error.message.indexOf('404') === -1) return cb(error)
                if (symbols) {
                  error.message = 'Failed to find Electron symbols v' + version + ' for ' + platform + '-' + arch + ' at ' + url
                } else {
                  error.message = 'Failed to find Electron v' + version + ' for ' + platform + '-' + arch + ' at ' + url
                }
                console.info('Download error : ' + error) 
                return cb(error)
              }
              // when dl is done then put in cache
              //debug('moving zip to', cachedZip)
              mv(path.join(tmpdir, filename), cachedZip, function (err) {
                if (err) return cb(err)
                cb(null, cachedZip)
              })
            })
          })
        })
	}
  
  function mkCacheDir (cb) {
    mkdir(cache, function (err) {
      if (err) {
        if (err.code !== 'EACCES') return cb(err)
        // try local folder if homedir is off limits (e.g. some linuxes return '/' as homedir)
        var localCache = path.resolve('./.electron')
        return mkdir(localCache, function (err) {
          if (err) return cb(err)
          cb(null, localCache)
        })
      }
      cb(null, cache)
    })
  }
}
