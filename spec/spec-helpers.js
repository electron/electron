exports.ifit = (condition) => (condition ? it : it.skip)
exports.ifdescribe = (condition) => (condition ? describe : describe.skip)
