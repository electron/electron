'use strict'

// interpret the value as a boolean, if possible
function interpretValue (value) {
  value = (value === 'yes' || value === '1') ? true : (value === 'no' || value === '0') ? false : value
  return value
}

// parses a feature string that has the format used in window.open()
// - `features` input string
// - `emit` function(key, value) - called for each parsed KV
module.exports = function parseFeaturesString (features, emit) {
  features = `${features}`.trim()

  if (!features) return

  let inArrayValue = false // mark whether in []
  let value = ''
  let arrayValue = []
  let key = ''
  let inGetKey = true // mark whether get key now

  const { length } = features
  for (let i = 0; i < length; i++) {
    const char = features[i]
    if (char === ' ' && !inArrayValue) {
      continue
    } else if (char === ',') {
      if (!inArrayValue) {
        inGetKey = true
        if (arrayValue.length) {
          emit(key, arrayValue)
          arrayValue = []
        } else {
          emit(key, interpretValue(value))
        }
        key = ''
      } else {
        arrayValue.push(value)
      }
      value = ''
    } else if (char === '=' && !inArrayValue) {
      inGetKey = false
      value = ''
    } else if (char === '[') {
      inArrayValue = true
      arrayValue = []
    } else if (char === ']') {
      inArrayValue = false
      arrayValue.push(value)
      value = ''
    } else if (inGetKey) {
      key += char
    } else {
      value += char
    }
  }

  if (key) {
    emit(key, arrayValue.length ? arrayValue : interpretValue(value))
  }
}
