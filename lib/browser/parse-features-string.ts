import { BrowserWindowConstructorOptions } from 'electron/main';

type RequiredBrowserWindowConstructorOptions = Required<BrowserWindowConstructorOptions>;
type IntegerBrowserWindowOptionKeys = {
  [K in keyof RequiredBrowserWindowConstructorOptions]:
    RequiredBrowserWindowConstructorOptions[K] extends number ? K : never
}[keyof RequiredBrowserWindowConstructorOptions];

const keysOfTypeNumber: IntegerBrowserWindowOptionKeys[] = [
  'x', 'y', 'width', 'height', 'minWidth', 'maxWidth', 'minHeight', 'maxHeight', 'opacity'
];

type CoercedValue = string | number | boolean;
function coerce(key: string, value: string): CoercedValue {
  if (keysOfTypeNumber.includes(key as IntegerBrowserWindowOptionKeys)) {
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

export function parseCommaSeparatedKeyValue(source: string): Record<string, CoercedValue> {
  const parsed: Record<string, CoercedValue> = {};
  const keyValuePairList = source.split(',');

  for (const keyValuePair of keyValuePairList) {
    const [key, value] = keyValuePair.split('=').map(str => str.trim());
    if (key) {
      parsed[key] = coerce(key, value);
    }
  }

  return parsed;
}

export function parseWebViewWebPreferences(preferences: string): Record<string, CoercedValue> {
  return parseCommaSeparatedKeyValue(preferences);
}

const allowedWebPreferences = ['zoomFactor', 'nodeIntegration', 'javascript', 'contextIsolation', 'webviewTag'] as const;
type AllowedWebPreference = typeof allowedWebPreferences[number];

export function parseFeatures(features: string): { options: Record<string, CoercedValue>; webPreferences: Record<AllowedWebPreference, CoercedValue> } {
  const parsed = parseCommaSeparatedKeyValue(features);
  const webPreferences: Record<AllowedWebPreference, CoercedValue> = {};

  for (const key of allowedWebPreferences) {
    if (parsed[key] !== undefined) {
      webPreferences[key] = parsed[key];
      delete parsed[key];
    }
  }

  if (parsed.left !== undefined) parsed.x = parsed.left;
  if (parsed.top !== undefined) parsed.y = parsed.top;

  return {
    options: parsed,
    webPreferences
  };
}
