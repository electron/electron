import { registerHooks } from 'node:module';

registerHooks({
  load(url, context, nextLoad) {
    return nextLoad(url, context);
  }
});
