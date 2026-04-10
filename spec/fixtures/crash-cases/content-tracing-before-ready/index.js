const { app, contentTracing } = require('electron');

const { expect } = require('chai');

(async () => {
    // Before app is ready, all contentTracing methods should reject
    // instead of crashing.
    if (!app.isReady()) {
        await expect(
            contentTracing.startRecording({ included_categories: ['*'] })
        ).to.be.rejectedWith(/before app is ready/);

        await expect(
            contentTracing.stopRecording()
        ).to.be.rejectedWith(/before app is ready/);

        await expect(
            contentTracing.getCategories()
        ).to.be.rejectedWith(/before app is ready/);

        await expect(
            contentTracing.getTraceBufferUsage()
        ).to.be.rejectedWith(/before app is ready/);
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
