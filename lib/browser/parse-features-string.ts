/**
 * Utilities to parse comma-separated key value pairs used in browser APIs.
 * For example: "x=100,y=200,width=500,height=500"
 */
import { BrowserWindowConstructorOptions } from 'electron/main';

type RequiredBrowserWindowConstructorOptions = Required<BrowserWindowConstructorOptions>;
type IntegerBrowserWindowOptionKeys = {
  [K in keyof RequiredBrowserWindowConstructorOptions]:
    RequiredBrowserWindowConstructorOptions[K] extends number ? K : never
}[keyof RequiredBrowserWindowConstructorOptions];

// This could be an array of keys, but an object allows us to add a compile-time
// check validating that we haven't added an integer property to
// BrowserWindowConstructorOptions that this module doesn't know about.
const keysOfTypeNumberCompileTimeCheck: { [K in IntegerBrowserWindowOptionKeys] : true } = {
  x: true,
  y: true,
  width: true,
  height: true,
  minWidth: true,
  maxWidth: true,
  minHeight: true,
  maxHeight: true,
  opacity: true
};
// Note `top` / `left` are special cases from the browser which we later convert
// to y / x.
const keysOfTypeNumber = new Set(['top', 'left', ...Object.keys(keysOfTypeNumberCompileTimeCheck)]);

/**
 * Note that we only allow "0" and "1" boolean conversion when the type is known
 * not to be an integer.
 *
 * The coercion of yes/no/1/0 represents best effort accordance with the spec:
 * https://html.spec.whatwg.org/multipage/window-object.html#concept-window-open-features-parse-boolean
 */
type CoercedValue = string | number | boolean;
function coerce (key: string, value: string): CoercedValue {
  if (keysOfTypeNumber.has(key)) {
    return parseInt(value, 10);
  }

  switch (value) {
    case 'true':
    case '1':
    case 'yes':
    case undefined:
      return true;
    case 'false':
    case '0':
    case 'no':
      return false;
    default:
      return value;
  }
}

export function parseCommaSeparatedKeyValue (source: string) {
  const parsed = {} as { [key: string]: any };
  for (const keyValuePair of source.split(',')) {
    const [key, value] = keyValuePair.split('=').map(str => str.trim());
    if (key) { parsed[key] = coerce(key, value); }
  }

  return parsed;
}

export function parseWebViewWebPreferences (preferences: string) {
  return parseCommaSeparatedKeyValue(preferences);
}

const allowedWebPreferences = ['zoomFactor', 'nodeIntegration', 'javascript', 'contextIsolation', 'webviewTag'] as const;
type AllowedWebPreference = (typeof allowedWebPreferences)[number];

/**
 * Parses a feature string that has the format used in window.open().
 */
export function parseFeatures (features: string) {
  const parsed = parseCommaSeparatedKeyValue(features);

  const webPreferences: { [K in AllowedWebPreference]?: any } = {};
  for (const key of allowedWebPreferences) {
    if (parsed[key] === undefined) continue;
    webPreferences[key] = parsed[key];
    delete parsed[key];
  }

  if (parsed.left !== undefined) parsed.x = parsed.left;
  if (parsed.top !== undefined) parsed.y = parsed.top;

  return {
    options: parsed as Omit<BrowserWindowConstructorOptions, 'webPreferences'>,
    webPreferences
  };
}
