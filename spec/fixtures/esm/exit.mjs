export function exitWithApp (app) {
  console.log('Exit with app, ready:', app.isReady());
  process.exit(0);
}
