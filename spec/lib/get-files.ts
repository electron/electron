import { once } from 'events';
import * as walkdir from 'walkdir';

/**
 * Walks the files in a directory and returns a list of files. Optionally, a
 * filter function can be provided to control which files are included.
 */
export async function getFiles (
  directoryPath: string,
  { filter = null }: { filter?: ((file: string) => boolean) | null } = {}
): Promise<string[]> {
  const files: string[] = [];
  const walker = walkdir(directoryPath, {
    no_recurse: true
  });
  walker.on('file', (file) => {
    if (!filter || filter(file)) {
      files.push(file);
    }
  });
  await once(walker, 'end');
  return files;
}
