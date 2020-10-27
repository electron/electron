exports.ifit = (condition) => (condition ? it : it.skip);
exports.ifdescribe = (condition) => (condition ? describe : describe.skip);

exports.delay = (time = 0) => new Promise(resolve => setTimeout(resolve, time));
