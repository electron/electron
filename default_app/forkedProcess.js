const os = require('os')
const path = require('path')

let productName = 'Child Product';
let companyName = 'Child Company';
let tmpPath = path.join(os.tmpdir(), productName + ' Crashes');

process.startCrashReporter(productName, companyName, 'http://localhost:8080/uploadDump/childDump', tmpPath, {
    randomData1: 'The Holidays are here!',
    randomData2: 'Happy New Year!'
});
process.crash();
