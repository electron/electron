function hello () {
}
hello.toString = 'hello';
module.exports = { functionWithToStringProperty: hello };
