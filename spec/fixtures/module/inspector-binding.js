const inspector = require('node:inspector');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

// This test case will set a breakpoint 4 lines below
function debuggedFunction () {
  let i;
  let accum = 0;
  for (i = 0; i < 5; i++) {
    accum += i;
  }
  return accum;
}

let scopeCallback = null;

function checkScope (session, scopeId) {
  session.post('Runtime.getProperties', {
    objectId: scopeId,
    ownProperties: false,
    accessorPropertiesOnly: false,
    generatePreview: true
  }, scopeCallback);
}

function debuggerPausedCallback (session, notification) {
  const params = notification.params;
  const callFrame = params.callFrames[0];
  const scopeId = callFrame.scopeChain[0].object.objectId;
  checkScope(session, scopeId);
}

function testSampleDebugSession () {
  let cur = 0;
  const failures = [];
  const expects = {
    i: [0, 1, 2, 3, 4],
    accum: [0, 0, 1, 3, 6]
  };
  scopeCallback = function (error, result) {
    if (error) failures.push(error);
    const i = cur++;
    let v, actual, expected;
    for (v of result.result) {
      actual = v.value.value;
      expected = expects[v.name][i];
      if (actual !== expected) {
        failures.push(`Iteration ${i} variable: ${v.name} ` +
          `expected: ${expected} actual: ${actual}`);
      }
    }
  };
  const session = new inspector.Session();
  session.connect();
  session.on('Debugger.paused',
    (notification) => debuggerPausedCallback(session, notification));
  let cbAsSecondArgCalled = false;
  session.post('Debugger.enable', () => { cbAsSecondArgCalled = true; });
  session.post('Debugger.setBreakpointByUrl', {
    lineNumber: 9,
    url: pathToFileURL(path.resolve(__dirname, __filename)).toString(),
    columnNumber: 0,
    condition: ''
  });

  debuggedFunction();
  scopeCallback = null;
  session.disconnect();
  process.send({
    cmd: 'assert',
    debuggerEnabled: cbAsSecondArgCalled,
    success: (cur === 5) && (failures.length === 0)
  });
}

testSampleDebugSession();
