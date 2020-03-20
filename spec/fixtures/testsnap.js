// taken from https://chromium.googlesource.com/v8/v8.git/+/HEAD/test/cctest/test-serialize.cc#1127
function f () { return g() * 2; } // eslint-disable-line no-unused-vars
function g () { return 43; }
