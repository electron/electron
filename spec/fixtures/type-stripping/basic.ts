import { app } from 'electron/main';

const logMessage = (message: string): void => console.log(message);

logMessage('running');

app.exit(0);
