import * as walkdir from 'walkdir';
import { emittedOnce } from './lib/events-helpers';

export async function getFiles (directoryPath: string, { filter = null }: {filter?: ((file: string) => boolean) | null} = {}) {
  const files: string[] = [];
  const walker = walkdir(directoryPath, {
    no_recurse: true
  });
  walker.on('file', (file) => {
    if (!filter || filter(file)) { files.push(file); }
  });
  await emittedOnce(walker, 'end');
  return files;
}
