function EventEmitter2() {};

EventEmitter2.prototype.on = function (event, fn) {
  this._callbacks = this._callbacks || {};
  (this._callbacks['$' + event] = this._callbacks['$' + event] || [])
    .push(fn);
  return this;
};

EventEmitter2.prototype.once = function (event, fn) {
  function on() {
    this.off(event, on);
    fn.apply(this, arguments);
  }

  on.fn = fn;
  this.on(event, on);
  return this;
};

EventEmitter2.prototype.off = function off(event, fn) {
  this._callbacks = this._callbacks || {};
  var callbacks = this._callbacks['$' + event];

  if (callbacks) {
    if (!fn) this._callbacks['$' + event] = []; // remove all _callbacks of given event
    else {
      var index = callbacks.indexOf(fn);
      if (index >= 0) callbacks.splice(index, 1);
    }
  }

  return this;
};

EventEmitter2.prototype.emit = function(event) {
  this._callbacks = this._callbacks || {};
  var args = [].slice.call(arguments, 1), callbacks = this._callbacks['$' + event];

  if (callbacks) {
    callbacks = callbacks.slice(0);
    for (var i = 0, len = callbacks.length; i < len; ++i) {
      callbacks[i].apply(this, args);
    }
  }

  return this;
};

EventEmitter2.prototype.listeners = function(event){
  this._callbacks = this._callbacks || {};
  return this._callbacks['$' + event] || [];
};

EventEmitter2.prototype.hasListeners = function(event){
  return !!this.listeners(event).length;
};

exports.EventEmitter = EventEmitter2;
