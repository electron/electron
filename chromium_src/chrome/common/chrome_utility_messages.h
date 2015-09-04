// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included message file, so no include guard.

#if defined(OS_WIN)
#include <Windows.h>
#endif  // defined(OS_WIN)

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "base/tuple.h"
#include "base/values.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

// Singly-included section for typedefs.
#ifndef CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_
#define CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_

#if defined(OS_WIN)
// A vector of filters, each being a Tuple containing a display string (i.e.
// "Text Files") and a filter pattern (i.e. "*.txt").
typedef std::vector<base::Tuple<base::string16, base::string16>>
    GetOpenFileNameFilter;
#endif  // OS_WIN

#endif  // CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_

#define IPC_MESSAGE_START ChromeUtilityMsgStart


#if defined(OS_WIN)
IPC_STRUCT_BEGIN(ChromeUtilityMsg_GetSaveFileName_Params)
  IPC_STRUCT_MEMBER(HWND, owner)
  IPC_STRUCT_MEMBER(DWORD, flags)
  IPC_STRUCT_MEMBER(GetOpenFileNameFilter, filters)
  IPC_STRUCT_MEMBER(int, one_based_filter_index)
  IPC_STRUCT_MEMBER(base::FilePath, suggested_filename)
  IPC_STRUCT_MEMBER(base::FilePath, initial_directory)
  IPC_STRUCT_MEMBER(base::string16, default_extension)
IPC_STRUCT_END()
#endif  // OS_WIN

//------------------------------------------------------------------------------
// Utility process messages:
// These are messages from the browser to the utility process.

// Tell the utility process to parse a JSON string into a Value object.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseJSON,
                     std::string /* JSON to parse */)

// Tell the utility process to decode the given image data.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_DecodeImage,
                     std::vector<unsigned char> /* encoded image contents */,
                     bool /* shrink image if needed for IPC msg limit */)

// Tell the utility process to decode the given JPEG image data with a robust
// libjpeg codec.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_RobustJPEGDecodeImage,
                     std::vector<unsigned char>)  // encoded image contents

// Tell the utility process to patch the given |input_file| using |patch_file|
// and place the output in |output_file|. The patch should use the bsdiff
// algorithm (Courgette's version).
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_PatchFileBsdiff,
                     base::FilePath /* input_file */,
                     base::FilePath /* patch_file */,
                     base::FilePath /* output_file */)

// Tell the utility process to patch the given |input_file| using |patch_file|
// and place the output in |output_file|. The patch should use the Courgette
// algorithm.
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_PatchFileCourgette,
                     base::FilePath /* input_file */,
                     base::FilePath /* patch_file */,
                     base::FilePath /* output_file */)


// Requests the utility process to respond with a
// ChromeUtilityHostMsg_ProcessStarted message once it has started.  This may
// be used if the host process needs a handle to the running utility process.
IPC_MESSAGE_CONTROL0(ChromeUtilityMsg_StartupPing)


#if defined(OS_WIN)
// Invokes ui::base::win::OpenFileViaShell from the utility process.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_OpenFileViaShell,
                     base::FilePath /* full_path */)

// Invokes ui::base::win::OpenFolderViaShell from the utility process.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_OpenFolderViaShell,
                     base::FilePath /* full_path */)

// Instructs the utility process to invoke GetOpenFileName. |owner| is the
// parent of the modal dialog, |flags| are OFN_* flags. |filter| constrains the
// user's file choices. |initial_directory| and |filename| select the directory
// to be displayed and the file to be initially selected.
//
// Either ChromeUtilityHostMsg_GetOpenFileName_Failed or
// ChromeUtilityHostMsg_GetOpenFileName_Result will be returned when the
// operation completes whether due to error or user action.
IPC_MESSAGE_CONTROL5(ChromeUtilityMsg_GetOpenFileName,
                     HWND /* owner */,
                     DWORD /* flags */,
                     GetOpenFileNameFilter /* filter */,
                     base::FilePath /* initial_directory */,
                     base::FilePath /* filename */)
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_GetSaveFileName,
                     ChromeUtilityMsg_GetSaveFileName_Params /* params */)
#endif  // defined(OS_WIN)


//------------------------------------------------------------------------------
// Utility process host messages:
// These are messages from the utility process to the browser.

// Reply when the utility process successfully parsed a JSON string.
//
// WARNING: The result can be of any Value subclass type, but we can't easily
// pass indeterminate value types by const object reference with our IPC macros,
// so we put the result Value into a ListValue. Handlers should examine the
// first (and only) element of the ListValue for the actual result.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseJSON_Succeeded,
                     base::ListValue)

// Reply when the utility process failed in parsing a JSON string.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseJSON_Failed,
                     std::string /* error message, if any*/)

// Reply when the utility process has failed while unpacking and parsing a
// web resource.  |error_message| is a user-readable explanation of what
// went wrong.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackWebResource_Failed,
                     std::string /* error_message, if any */)

// Reply when the utility process has succeeded in decoding the image.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_DecodeImage_Succeeded,
                     SkBitmap)  // decoded image

// Reply when an error occurred decoding the image.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_DecodeImage_Failed)

// Reply when a file has been patched.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_PatchFile_Finished, int /* result */)


// Reply when the utility process has started.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_ProcessStarted)


#if defined(OS_WIN)
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_GetOpenFileName_Failed)
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GetOpenFileName_Result,
                     base::FilePath /* directory */,
                     std::vector<base::FilePath> /* filenames */)
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_GetSaveFileName_Failed)
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GetSaveFileName_Result,
                     base::FilePath /* path */,
                     int /* one_based_filter_index  */)
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_BuildDirectWriteFontCache,
                     base::FilePath /* cache file path */)
#endif  // defined(OS_WIN)
