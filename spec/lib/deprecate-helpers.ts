import { expect } from 'chai';

export async function expectDeprecationMessages (func: () => any, ...expected: string[]) {
  const messages: string[] = [];

  const originalWarn = console.warn;
  console.warn = (message) => {
    messages.push(message);
  };

  const warningListener = (error: Error) => {
    messages.push(error.message);
  };

  process.on('warning', warningListener);

  try {
    return await func();
  } finally {
    // process.emitWarning seems to need us to wait a tick
    await new Promise(process.nextTick);
    console.warn = originalWarn;
    process.off('warning' as any, warningListener);
    expect(messages).to.deep.equal(expected);
  }
}
