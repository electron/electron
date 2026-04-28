const { app, contentTracing } = require('electron');

const assert = require('node:assert/strict');

(async () => {
  // Before app is ready, all contentTracing methods should reject
  // instead of crashing.
  if (!app.isReady()) {
    await assert.rejects(() => contentTracing.startRecording({ included_categories: ['*'] }), /before app is ready/);

    await assert.rejects(() => contentTracing.stopRecording(), /before app is ready/);

    await assert.rejects(() => contentTracing.getCategories(), /before app is ready/);

    await assert.rejects(() => contentTracing.getTraceBufferUsage(), /before app is ready/);
  }

  await app.whenReady();

  // After app is ready, startRecording should work normally.
  await contentTracing.startRecording({ included_categories: ['*'] });
  await contentTracing.stopRecording();
})()
  .then(app.quit)
  .catch((err) => {
    console.error(err);
    app.exit(1);
  });
