var services = {
  passwordSavingEnabled: {
    get: function(details, cb) {
      cb({level_of_control: "not_controllable"});
    },
    set: function() {}
  },
  autofillEnabled: {
    get: function(details, cb) {
      cb({level_of_control: "not_controllable"});
    },
    set: function() {}
  }
}

var binding = {
  services
}

exports.binding = binding;
