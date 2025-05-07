import { expect } from 'chai';

type ExpectedWarningMessage = RegExp | string;

async function expectWarnings (
  func: () => any,
  expected: { name: string, message: ExpectedWarningMessage }[]
) {
  const messages: { name: string, message: string }[] = [];

  const originalWarn = console.warn;
  console.warn = (message) => {
    messages.push({ name: 'Warning', message });
  };

  const warningListener = (error: Error) => {
    messages.push({ name: error.name, message: error.message });
  };

  process.on('warning', warningListener);

  try {
    return await func();
  } finally {
    // process.emitWarning seems to need us to wait a tick
    await new Promise(process.nextTick);
    console.warn = originalWarn;
    process.off('warning' as any, warningListener);
    expect(messages).to.have.lengthOf(expected.length);
    for (const [idx, { name, message }] of messages.entries()) {
      expect(name).to.equal(expected[idx].name);
      if (expected[idx].message instanceof RegExp) {
        expect(message).to.match(expected[idx].message);
      } else {
        expect(message).to.equal(expected[idx].message);
      }
    }
  }
}

export async function expectWarningMessages (
  func: () => any,
  ...expected: ({ name: string, message: ExpectedWarningMessage } | ExpectedWarningMessage)[]
) {
  return expectWarnings(func, expected.map((message) => {
    if (typeof message === 'string' || message instanceof RegExp) {
      return { name: 'Warning', message };
    } else {
      return message;
    }
  }));
}

export async function expectDeprecationMessages (
  func: () => any, ...expected: ExpectedWarningMessage[]
) {
  return expectWarnings(
    func, expected.map((message) => ({ name: 'DeprecationWarning', message }))
  );
}
