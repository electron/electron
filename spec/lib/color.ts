import { assert } from 'chai';

const DEFAULT_COLOR_THRESHOLD = 90;

/**
 * Common colors used by specs.
 */
export const HexColors = {
  GREEN: '#00b140',
  PURPLE: '#6a0dad',
  RED: '#ff0000',
  BLUE: '#0000ff',
  WHITE: '#ffffff'
};

/**
 * Represents a color in either RGB or sRGB color space, depending on context.
 */
export interface Color {
  r: number;
  g: number;
  b: number;
}

/**
 * Parses a 6-digit hexidecimal number (with an optional leading `#`) as a color
 * in RGB order.
 */
export function colorFromHex (hexColor: string): Color {
  const digits = hexColor.startsWith('#') ? hexColor.slice(1) : hexColor;
  if (digits.length !== 6) {
    throw new Error('hex color must be exactly 6 digits');
  }

  return {
    r: parseHexOrThrow(digits.slice(0, 2)),
    g: parseHexOrThrow(digits.slice(2, 4)),
    b: parseHexOrThrow(digits.slice(4, 6))
  };
}

/**
 * Displays a color as a 6-digit hexidecimal number with a leading `#`.
 */
function colorAsHexStr (color: Color): string {
  return (
    '#' +
    [color.r, color.b, color.g]
      .map((n) => n.toString(16).padStart(2, '0'))
      .join('')
  );
}

/**
 * Compare if two colors are within a threshold of difference, indicating they
 * are similar enough to be considered the same.
 *
 * The threshold value can be understood based on the comparison heuristic. This
 * function compares colors by their euclidean distance in color space. Put
 * another way, if both colors are viewed as 3D points in space, the threshold
 * is the maximum distance (literally) between those points where they can still
 * be considered close enough to represent the same point (that is, the same
 * color).
 *
 * ### Assertions
 *
 * When asserting that colors are similar or dissimilar, consider using the
 * `expectColorsAreSimilar` and `expectColorsAreDissimilar` functions as they
 * provide more helpful assertion error messages.
 */
export function areColorsSimilar (
  color1: Color | string,
  color2: Color | string,
  threshold: number = DEFAULT_COLOR_THRESHOLD
): boolean {
  // Coerce both colors into their numeric components
  const {
    r: r1,
    g: g1,
    b: b1
  } = typeof color1 === 'string' ? colorFromHex(color1) : color1;
  const {
    r: r2,
    g: g2,
    b: b2
  } = typeof color2 === 'string' ? colorFromHex(color2) : color2;

  // Calculate the euclidean distance between both colors.
  const distance = Math.sqrt(
    Math.pow(r1 - r2, 2) + Math.pow(g1 - g2, 2) + Math.pow(b1 - b2, 2)
  );

  return distance <= threshold;
}

/**
 * Convenience function that asserts that two colors are within a threshold of
 * difference, showing a helpful assertion error message if they are not.
 *
 * See `areColorsSimilar` for more information on the threshold value.
 */
export function expectColorsAreSimilar (
  color1: Color | string,
  color2: Color | string,
  threshold: number = DEFAULT_COLOR_THRESHOLD
) {
  const color1str = typeof color1 === 'string' ? color1 : colorAsHexStr(color1);
  const color2str = typeof color2 === 'string' ? color2 : colorAsHexStr(color2);

  assert(
    areColorsSimilar(color1, color2, threshold),
    `expected ${color1str} to be similar (threshold = ${threshold}) to ${color2str}`
  );
}

/**
 * Convenience function that asserts that two colors are not within a threshold
 * of difference, showing a helpful assertion error message if they are.
 *
 * See `areColorsSimilar` for more information on the threshold value.
 */
export function expectColorsAreDissimilar (
  color1: Color | string,
  color2: Color | string,
  threshold: number = DEFAULT_COLOR_THRESHOLD
) {
  const color1str = typeof color1 === 'string' ? color1 : colorAsHexStr(color1);
  const color2str = typeof color2 === 'string' ? color2 : colorAsHexStr(color2);

  assert(
    !areColorsSimilar(color1, color2, threshold),
    `expected ${color1str} to *not* be similar (threshold = ${threshold}) to ${color2str}`
  );
}

/**
 * Parses a hexidecimal string as an integer, throwing if it cannot be parsed.
 */
function parseHexOrThrow (numAsHex: string): number {
  const num = parseInt(numAsHex, 16);
  if (isNaN(num)) {
    throw new Error(`invalid hexidecimal number: '${numAsHex}'`);
  }
  return num;
}
