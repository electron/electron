#!/usr/bin/env node

var electron = require('./')

var proc = require('child_process')

proc.spawn(electron, process.argv.slice(2), {stdio: 'inherit'});
