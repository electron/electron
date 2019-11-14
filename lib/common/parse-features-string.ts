/**
 * Utilities to parse comma-separated key value pairs used in browser APIs.
 * For example: "x=100,y=200,width=500,height=500"
 */
import { BrowserWindowConstructorOptions } from 'electron'

type A = Required<BrowserWindowConstructorOptions>
type KeysOfTypeInteger = {
  [K in keyof A]:
    A[K] extends number ? K : never
}[keyof A]

// This could be an array of keys, but an object allows us to add a compile-time
// check validating that we haven't added an integer property to
// BrowserWindowConstructorOptions that this module doesn't know about.
const keysOfTypeNumberCompileTimeCheck: { [K in KeysOfTypeInteger] : true } = {
  x: true,
  y: true,
  width: true,
  height: true,
  minWidth: true,
  maxWidth: true,
  minHeight: true,
  maxHeight: true,
  opacity: true
}
// Note `top` / `left` are special cases from the browser which we later convert
// to y / x.
const keysOfTypeNumber = ['top', 'left', ...Object.keys(keysOfTypeNumberCompileTimeCheck)]

/**
 * Note that we only allow "0" and "1" boolean conversion when the type is known
 * not to be an integer.
 *
 * The coercion of yes/no/1/0 represents best effort accordance with the spec:
 * https://html.spec.whatwg.org/multipage/window-object.html#concept-window-open-features-parse-boolean
 */
type CoercedValue = string | number | boolean
function coerce (key: string, value: string): CoercedValue {
  if (keysOfTypeNumber.includes(key)) {
    return Number(value)
  }

  switch (value) {
    case 'true':
    case '1':
    case 'yes':
    case undefined:
      return true
    case 'false':
    case '0':
    case 'no':
      return false
    default:
      return value
  }
}

function parseCommaSeparatedKeyValue (source: string, useDeprecatedBehaviorForBareKeys: boolean) {
  const bareKeys = [] as string[]
  const parsed = source.split(',').reduce((map, keyValuePair) => {
    const [key, value] = keyValuePair.split('=').map(str => str.trim())

    if (useDeprecatedBehaviorForBareKeys && value === undefined) {
      bareKeys.push(key)
      return map
    }

    map[key] = coerce(key, value)
    return map
  }, {} as { [key: string]: any })

  return { parsed, bareKeys }
}

export function parseWebViewWebPreferences (preferences: string) {
  return parseCommaSeparatedKeyValue(preferences, false).parsed
}

const allowedWebPreferences = ['zoomFactor', 'nodeIntegration', 'enableRemoteModule', 'preload', 'javascript', 'contextIsolation', 'webviewTag']

/**
 * Parses a feature string that has the format used in window.open().
 *
 * `useDeprecatedBehaviorForBareKeys` — In the html spec, windowFeatures keys
 * without values are interpreted as `true`. Previous versions of Electron did
 * not respect this. In order to not break any applications, this will be
 * flipped in the next major version.
 */
export function parseFeatures (
  features: string,
  useDeprecatedBehaviorForBareKeys: boolean
): {
  options: Omit<BrowserWindowConstructorOptions, 'webPreferences'> & { [key: string]: CoercedValue };
  webPreferences: BrowserWindowConstructorOptions['webPreferences'];
  additionalFeatures: string[];
} {
  const { parsed, bareKeys } = parseCommaSeparatedKeyValue(features, useDeprecatedBehaviorForBareKeys)

  const webPreferences = {}
  allowedWebPreferences.forEach((key) => {
    if (parsed[key] === undefined) return
    (webPreferences as any)[key] = parsed[key]
    delete parsed[key]
  })

  if (parsed.left !== undefined) parsed.x = parsed.left
  if (parsed.top !== undefined) parsed.y = parsed.top

  return {
    options: parsed,
    webPreferences,
    additionalFeatures: bareKeys
  }
}
