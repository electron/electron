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
 * Compare if two colors are withing a threshold of difference, indicating they
 * are similar enough to be considered the same.
 *
 * The threshold value can be understood based on the comparison heuristic. This
 * function compares colors by their euclidean distance in color space. Put
 * another way, if both colors are viewed as 3D points in space, the threshold
 * is the maximum distance (literally) between those points where they can still
 * be considered close enough to represent the same point (that is, the same
 * color).
 */
export function areColorsSimilar (
  color1: Color | string,
  color2: Color | string,
  threshold: number = 90
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
 * Parses a hexidecimal string as an integer, throwing if it cannot be parsed.
 */
function parseHexOrThrow (numAsHex: string): number {
  const num = parseInt(numAsHex, 16);
  if (isNaN(num)) {
    throw new Error(`invalid hexidecimal number: '${numAsHex}'`);
  }
  return num;
}

/*
TODO: inline this to the file where it's used:
export enum HexColors {
  GREEN = '#00b140',
  PURPLE = '#6a0dad',
  RED = '#ff0000',
  BLUE = '#0000ff'
};
*/
