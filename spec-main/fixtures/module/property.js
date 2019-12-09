exports.property = 1127

function func () {

}
func.property = 'foo'
exports.func = func

exports.getFunctionProperty = () => {
  return `${func.property}-${process.type}`
}
