const { app } = process._linkedBinding('electron_browser_app');

// Only one app object permitted.
export default app;
