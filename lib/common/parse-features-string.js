// parses a feature string that has the format used in window.open()
// - `str` input string
// - `emit` function(key, value) - called for each parsed KV
module.exports = function parseFeaturesString (str, emit) {
  // split the string by ','
  let i, len
  let strTokens = params.str.split(/,\s*/)
  for (i = 0, len = strTokens.length; i < len; i++) {
    // expected form is either a key by itself (true boolean flag)
    // or a key/value, in the form of 'key=value'
    // split the tokens by '='
    let kv = strTokens[i]
    let kvTokens = kv.split(/\s*=/)
    let key = kvTokens[0]
    let value = kvTokens[1]
    if (!key) continue

    // interpret the value as a boolean, if possible 
    value = (value === 'yes' || value === '1') ? true : (value === 'no' || value === '0') ? false : value

    // emit the parsed pair
    emit(key, value)
  }
}
