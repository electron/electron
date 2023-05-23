/**
 * This type represents approximately anything that can be stringified as JSON
 * via `JSON.stringify`. This helps catch errors when passing non-serializable
 * objects to the `jsont`-tagged template function.
 */
export type ToJson =
  | string
  | number
  | boolean
  | null
  | undefined // NOTE: `undefined` doesn't exist in JSON, but it *is* accepted by `JSON.stringify`.
  | readonly ToJson[]
  | {}
  | { toJSON(): string };

/**
 * An ergonomic and safe tagged template function for interpolating JSON values
 * into a string, particularly JavaScript code, but works for any general string
 * that needs JSON values interpolated in it.
 *
 * You can supply any object that can be converted to JSON (via
 * `JSON.stringify`) as a template argument. See the `ToJson` type for more
 * information on JSON arguments. It's best to think of interpolations as
 * "passing" full values into the script. For example, if you wanted to pass
 * a number `n` as a string, you should write `${n.toString()}` rather than
 * `'${n}'`.
 *
 * **Best practice**: This helper should only be used for short 'n' sweet
 * snippets of code. As a rule of thumb, if your snippet wouldn't fit in a
 * 280-character status update on your favorite social media platform, consider
 * writing your script as a fixture. This benefits reviewers and keeps our spec
 * code tidy.
 */
export function jsont (
  segments: TemplateStringsArray,
  ...args: readonly ToJson[]
): string {
  // Edge cases: no interpolations
  if (segments.length === 1) {
    // TODO: somehow warn that this is *probably* an anti-pattern.
    return segments[0];
  }

  // Reduce the segments and interpolations into a single string
  return segments
    .reduce((acc, segment, i) => {
      // Push the current segment
      acc.push(segment);

      // If this segment has an template argument following it, also push that
      // as a segment
      if (i < args.length) {
        acc.push(JSON.stringify(args[i]));
      }

      // Continue reducing
      return acc;
    }, [] as string[])
    .join('');
}
