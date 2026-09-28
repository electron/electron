declare let standardScheme: string;
declare let serviceWorkerScheme: string;

declare module 'dbus-native';

// The runners accept vitest's `(title, options, fn)` form on top of mocha's
// `(title, fn)`; see spec/index.js and spec/vitest/mocha-compat.ts.
interface ElectronSpecOptions {
  /** Timeout in milliseconds for the test, or for each test in the suite. */
  timeout?: number;
  /** How many times to retry a failing test. */
  retry?: number;
  /**
   * 'serial' marks suites that need the machine to themselves (window focus,
   * the clipboard, the screen, global shortcuts); the vitest runner runs them
   * one at a time after everything else.
   */
  tags?: 'serial'[] | 'serial';
}

declare namespace Mocha {
  interface SuiteFunction {
    (title: string, options: ElectronSpecOptions, fn: (this: Suite) => void): Suite;
  }
  interface PendingSuiteFunction {
    (title: string, options: ElectronSpecOptions, fn: (this: Suite) => void): Suite | void;
  }
  interface ExclusiveSuiteFunction {
    (title: string, options: ElectronSpecOptions, fn: (this: Suite) => void): Suite;
  }
  interface TestFunction {
    (title: string, options: ElectronSpecOptions, fn: Func | AsyncFunc): Test;
  }
  interface PendingTestFunction {
    (title: string, options: ElectronSpecOptions, fn: Func | AsyncFunc): Test;
  }
  interface ExclusiveTestFunction {
    (title: string, options: ElectronSpecOptions, fn: Func | AsyncFunc): Test;
  }
}
