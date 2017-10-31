#!/bin/bash

_project=$1
_branch=$2
_circle_token=$3

trigger_build_url=https://circleci.com/api/v1.1/project/github/${_project}/tree/${_branch}?circle-token=${_circle_token}

post_data=$(cat <<EOF
{
  "build_parameters": {
    "RUN_RELEASE_BUILD": "true",
    "CIRCLE_JOB": "electron-linux-arm"
  }
}
EOF)

curl \
--header "Accept: application/json" \
--header "Content-Type: application/json" \
--data "${post_data}" \
--request POST ${trigger_build_url}