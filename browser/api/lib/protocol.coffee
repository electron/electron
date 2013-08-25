module.exports = process.atomBinding 'protocol'

module.exports.RequestStringJob =
class RequestStringJob
  constructor: ({mimeType, charset, data}) ->
    if typeof data isnt 'string' and not data instanceof Buffer
      throw new TypeError('Data should be string or Buffer')

    @mimeType = mimeType ? 'text/plain'
    @charset = charset ? 'UTF-8'
    @data = String data

module.exports.RequestFileJob =
class RequestFileJob
  constructor: (@path) ->
