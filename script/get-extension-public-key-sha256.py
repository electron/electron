#!/usr/bin/python

# Extracts SubjectPublicKeyInfo and calculates the sha256 from it
# CRX file format: https://developer.chrome.com/extensions/crx
import struct
from hashlib import sha256
import base64
import sys

assert len(sys.argv) == 2, 'Usage: ./script/get-extension-public-key-sha256.py <crx-path>'
extension_path = sys.argv[1]

f = open(extension_path, 'rb')

magic_number = f.read(4)
assert (magic_number == 'Cr24'), 'Not a valid crx file'

version_buffer = f.read(4)
version = struct.unpack_from('i', version_buffer)[0]
assert version == 2, 'Only version 2 CRX is currently supported'

public_key_len_buffer = f.read(4)
public_key_len = struct.unpack_from('i', public_key_len_buffer)[0]

# skip past the sig length
f.seek(4, 1)

public_key_info = f.read(public_key_len)
pk_hash = sha256(public_key_info).hexdigest()

def format_hex_data(pk_hash):
  return ' '.join('0x' + pk_hash[i:i+2] + ',' for i in xrange(0,len(pk_hash), 2))[:-1]

# In groups of 8 of '0x0e, '
def group_hex(hex):
  return '\n'.join(hex[i:i+48] for i in xrange(0,len(hex), 48))


print 'public key len: ', public_key_len
print 'manifest key: ', base64.b64encode(public_key_info)
print 'public key: ', group_hex(format_hex_data(public_key_info.encode('hex')))
print 'pk hash sha256: ', group_hex(format_hex_data(pk_hash))
