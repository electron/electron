var contentSettings = requireNative('contentSettings');
var inIncognitoContext = requireNative('process').InIncognitoContext();

function getCurrent (contentType) {
  return contentSettings.getCurrent(contentType, inIncognitoContext)
}

function ContentSetting (contentType) {
  this.contentType = contentType;
}

ContentSetting.prototype.toString = function () {
  return getCurrent(this.contentType)
}

function getContentSetting(contentType) {
  return new ContentSetting(contentType)
}

var binding = new Proxy({}, {
  get: function(proxy, name) {
    if (contentSettings.getContentTypes().indexOf(name) !== -1)
      return getContentSetting(name)
  }
})

exports.binding = binding
