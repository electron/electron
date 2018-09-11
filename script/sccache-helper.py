#!/usr/bin/env python

import argparse
import os
import sys

import lib.sccache as sccache

def parse_args():
  parser = argparse.ArgumentParser(description='Run sccache`')

  parser.add_argument('--azure-container',
                      help='Name of azure container to use')
  parser.add_argument('--azure-connection',
                      help='Name of azure connection to use')
  parser.add_argument('--aws-access-key', help='S3 access key to use')
  parser.add_argument('--aws-secret', help='S3 secret access key to use')

  local_args, sccache_args = parser.parse_known_args()
  return local_args, sccache_args


def main():
  local_args, sccache_args = parse_args()

  if local_args.azure_container:
    os.environ['SCCACHE_AZURE_BLOB_CONTAINER'] = local_args.azure_container

  if local_args.azure_connection:
    os.environ['SCCACHE_AZURE_CONNECTION_STRING'] = local_args.azure_connection

  if local_args.aws_access_key:
    os.environ['AWS_ACCESS_KEY_ID'] = local_args.aws_access_key

  if local_args.aws_secret:
    os.environ['AWS_SECRET_ACCESS_KEY'] = local_args.aws_secret

  return sccache.run(*sccache_args)


if __name__ == '__main__':
    sys.exit(main())
