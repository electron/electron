import { join, resolve } from 'path';
import { pathToFileURL } from 'url';

/**
 * Absolute path to the root of the test `fixtures` directory.
 */
export const FIXTURES_PATH = resolve(__dirname, '..', 'fixtures');

/**
 * Joins the passed paths relative to the root of the `fixtures` directory. This
 * function returns an absolute path to the fixture directory, but does not
 * resolve the following paths. That is, `fixturePath('.', 'cats')` will result
 * in a path like `.../fixtures/./cats`.
 */
export function fixturePath (...paths: string[]): string {
  // NB: `resolve` cannot be used here as some tests rely on this function
  // keeping segments like `.` and `..` in the returned path
  return join(FIXTURES_PATH, ...paths);
}

/**
 * A descriptor of properties to be set on a `URL` object.
 *
 * This is mostly the same as the type `Partial<URL>` with some notable
 * exceptions:
 * - The read-only `origin` type is excluded since it cannot be assigned to.
 * - `URL` methods are excluded.
 * - `searchParams` accepts an object of key-value pairs describing search
 *   parameters to set on the URL's `searchParams` object.
 */
export interface URLProperties {
  hash?: string;
  host?: string;
  hostname?: string;
  href?: string;
  password?: string;
  pathname?: string;
  port?: string;
  protocol?: string;
  search?: string;
  searchParams?: Record<string, string>;
  username?: string;
}

/**
 * Applies the properties definied in a `URLProperties` descriptor to a `URL`
 * object.
 *
 * Note: this function mutates `url`, but also returns it for convenient use.
 */
function applyURLProperties (url: URL, props: URLProperties): URL {
  for (const key of Object.keys(props)) {
    if (key === 'searchParams') {
      // Search params has special handling since it has its own object type
      for (const searchKey of Object.keys(props.searchParams!)) {
        url.searchParams.set(searchKey, props.searchParams![searchKey]);
      }
    } else {
      // All other keys should be fine to assign directly
      type RemainingKeys = keyof Omit<URLProperties, 'searchParams'>;
      url[key as RemainingKeys] = props[key as RemainingKeys]!;
    }
  }

  return url;
}

/**
 * Convenience function that resolves the passed paths relative to the root of
 * the `fixtures` directory, then converts it to a `file://` URL.
 *
 * Optionally, the user may provide a properties object as the last argument.
 * This object is used to assign values to specific properties of the `URL`
 * object before it is turned into a string.
 *
 * Note: while it might make more sense to return the `URL` object from this
 * function, nearly all usages immediately convert it to a string, so that use
 * case is prioritized over what makes sense for this function in a vacuum.
 */
export function fixtureFileURL (
  ...args: string[] | [...string[], URLProperties]
): string {
  // Edge case: no arguments
  if (args.length === 0) {
    return pathToFileURL(FIXTURES_PATH).toString();
  }
  // Get the last argument passed to this function
  const lastArg = args[args.length - 1];

  // Check if the last arugment is a properties object
  const hasProperties = typeof lastArg === 'object';

  // Determine what properties should be set on the final URL and what arguments
  // make up the path segments
  const properties = hasProperties ? lastArg : {};
  const paths = hasProperties
    ? (args.slice(0, -1) as string[])
    : (args as string[]);

  // Get the fixture path, convert it to a `file://` URL, assign any properties
  // passed to this function, and finally convert it to a string
  return applyURLProperties(
    pathToFileURL(fixturePath(...paths)),
    properties
  ).toString();
}
