import * as walkdir from 'walkdir';

export async function getFiles (directoryPath: string, { filter = null }: {filter?: ((file: string) => boolean) | null} = {}) {
  const files: string[] = [];
  const walker = walkdir(directoryPath, {
    no_recurse: true
  });
  walker.on('file', (file) => {
    if (!filter || filter(file)) { files.push(file); }
  });
  await new Promise((resolve) => walker.on('end', resolve));
  return files;
}
