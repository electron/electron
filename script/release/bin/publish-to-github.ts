import { makeRelease } from '../release';

makeRelease().catch((err) => {
  console.error('Error occurred while making release:', err);
  process.exit(1);
});
