import { validateRelease } from '../release';

validateRelease().catch((err) => {
  console.error('Error occurred while validating release:', err);
  process.exit(1);
});
