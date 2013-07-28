#!/usr/bin/env coffee
# Usage:
# Copy the crash log into pasteboard and then run
# pbpaste | ./script/translate-crash-log-addresses.coffee

atos = (addresses, callback) ->
  path = require 'path'
  exec = require('child_process').exec

  exec 'atos -o vendor/brightray/vendor/download/libchromiumcontent/Release/libchromiumcontent.dylib -arch i386 '.concat(addresses...), (error, stdout, stderr) ->
    throw error if error?
    callback stdout.split('\n')

parse_stack_trace = (raw) ->
  lines = {}
  addresses = []
  for line in raw
    columns = line.split /\ +/
    if columns[1] == 'libchromiumcontent.dylib' and /0x[a-f0-9]+/.test columns[3]
      lines[columns[0]] = addresses.length
      addresses.push '0x' + parseInt(columns[5]).toString(16) + ' '

  atos addresses, (parsed) ->
    for line in raw
      columns = line.split /\ +/
      frame = columns[0]
      if lines[frame]?
        console.log frame, parsed[lines[frame]]
      else
        console.log line

parse_log_file = (content) ->
  state = 'start'
  stack_trace = []
  lines = content.split /\r?\n/

  for line in lines
    if state == 'start'
      if /Thread \d+ Crashed::/.test line
        console.log line
        state = 'parse'
    else if state == 'parse'
      break if line == ''
      stack_trace.push line

  parse_stack_trace stack_trace

input = ''
process.stdin.resume()
process.stdin.setEncoding 'utf8'
process.stdin.on 'data', (chunk) ->
  input += chunk
process.stdin.on 'end', ->
  parse_log_file input
