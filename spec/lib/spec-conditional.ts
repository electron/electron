export function ifit (cond: boolean): Mocha.TestFunction | Mocha.PendingTestFunction {
  return cond ? it : it.skip;
}

export function ifdescribe (cond: boolean): Mocha.SuiteFunction | Mocha.PendingSuiteFunction {
  return cond ? describe : describe.skip;
}
