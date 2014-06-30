#!/bin/bash

# Copyright (c) 2014 GitHub, Inc. All rights reserved.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

# usage: link_file_to_helper.sh relative-path-to-Contents
# This script links a file from the helper app to the main app.

FILE_PATH=$1
HELPER_CONTENTS_FOLDER_PATH="$BUILT_PRODUCTS_DIR/$CONTENTS_FOLDER_PATH/Frameworks/$PRODUCT_NAME Helper.app/Contents"

PARENT="`dirname "$HELPER_CONTENTS_FOLDER_PATH/$FILE_PATH"`"
if [[ ! -d "$PARENT" ]]; then
  mkdir -p "$PARENT"
fi

cd "$HELPER_CONTENTS_FOLDER_PATH"
ln -s "../../../../$FILE_PATH" "$FILE_PATH"
