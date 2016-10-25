// parses a feature string that has the format used in window.open()
// - `features` input string
// - `emit` function(key, value) - called for each parsed KV
module.exports = function parseFeaturesString (features, emit) {
  // split the string by ','
  features.split(/,\s*/).forEach((feature) => {
    // expected form is either a key by itself or a key/value pair in the form of
    // 'key=value'
    let [key, value] = feature.split(/\s*=/)
    if (!key) return

    // interpret the value as a boolean, if possible
    value = (value === 'yes' || value === '1') ? true : (value === 'no' || value === '0') ? false : value

    // emit the parsed pair
    emit(key, value)
  })
}
