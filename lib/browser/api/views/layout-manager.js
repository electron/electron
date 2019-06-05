'use strict'

const { LayoutManager } = process.electronBinding('layout_manager')

LayoutManager.prototype._init = function () {
}

module.exports = LayoutManager
