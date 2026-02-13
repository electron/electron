import { app } from 'electron/main';

enum Test {
  A,
  B,
  C,
}

console.log(Test.A);

app.exit(0);
