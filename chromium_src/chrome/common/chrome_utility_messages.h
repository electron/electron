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
#include "build/build_config.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

#if defined(FULL_SAFE_BROWSING)
#include "chrome/common/safe_browsing/ipc_protobuf_message_macros.h"
#include "chrome/common/safe_browsing/protobuf_message_param_traits.h"
#include "chrome/common/safe_browsing/zip_analyzer_results.h"
#endif

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

#if defined(FULL_SAFE_BROWSING)
IPC_ENUM_TRAITS_VALIDATE(
    safe_browsing::ClientDownloadRequest_DownloadType,
    safe_browsing::ClientDownloadRequest_DownloadType_IsValid(value))

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(safe_browsing::ClientDownloadRequest_Digests)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(sha256)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(sha1)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(md5)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_CertificateChain_Element)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(certificate)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_CertificateChain)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(element)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_SignatureInfo)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(certificate_chain)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_FUNDAMENTAL_MEMBER(trusted)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(signed_data)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_PEImageHeaders_DebugData)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(directory_entry)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(raw_data)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_PEImageHeaders)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(dos_header)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(file_header)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(optional_headers32)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(optional_headers64)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(section_header)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(export_section_data)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(debug_data)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_MachOHeaders_LoadCommand)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_FUNDAMENTAL_MEMBER(command_id)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(command)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_MachOHeaders)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(mach_header)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(load_commands)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_ImageHeaders)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(pe_headers)
  IPC_PROTOBUF_MESSAGE_TRAITS_REPEATED_COMPLEX_MEMBER(mach_o_headers)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_PROTOBUF_MESSAGE_TRAITS_BEGIN(
    safe_browsing::ClientDownloadRequest_ArchivedBinary)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(file_basename)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_FUNDAMENTAL_MEMBER(download_type)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(digests)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_FUNDAMENTAL_MEMBER(length)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(signature)
  IPC_PROTOBUF_MESSAGE_TRAITS_OPTIONAL_COMPLEX_MEMBER(image_headers)
IPC_PROTOBUF_MESSAGE_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(safe_browsing::zip_analyzer::Results)
  IPC_STRUCT_TRAITS_MEMBER(success)
  IPC_STRUCT_TRAITS_MEMBER(has_executable)
  IPC_STRUCT_TRAITS_MEMBER(has_archive)
  IPC_STRUCT_TRAITS_MEMBER(archived_binary)
  IPC_STRUCT_TRAITS_MEMBER(archived_archive_filenames)
IPC_STRUCT_TRAITS_END()
#endif  // FULL_SAFE_BROWSING

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

// Tell the utility process to decode the given image data.
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_DecodeImage,
                     std::vector<unsigned char> /* encoded image contents */,
                     bool /* shrink image if needed for IPC msg limit */,
                     int /* delegate id */)

#if defined(OS_CHROMEOS)
// Tell the utility process to decode the given JPEG image data with a robust
// libjpeg codec.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_RobustJPEGDecodeImage,
                     std::vector<unsigned char> /* encoded image contents*/,
                     int /* delegate id */)
#endif  // defined(OS_CHROMEOS)

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

#if defined(OS_CHROMEOS)
// Tell the utility process to create a zip file on the given list of files.
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_CreateZipFile,
                     base::FilePath /* src_dir */,
                     std::vector<base::FilePath> /* src_relative_paths */,
                     base::FileDescriptor /* dest_fd */)
#endif  // defined(OS_CHROMEOS)

// Requests the utility process to respond with a
// ChromeUtilityHostMsg_ProcessStarted message once it has started.  This may
// be used if the host process needs a handle to the running utility process.
IPC_MESSAGE_CONTROL0(ChromeUtilityMsg_StartupPing)

#if defined(FULL_SAFE_BROWSING)
// Tells the utility process to analyze a zip file for malicious download
// protection, providing a file that can be used temporarily to analyze binaries
// contained therein.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_AnalyzeZipFileForDownloadProtection,
                     IPC::PlatformFileForTransit /* zip_file */,
                     IPC::PlatformFileForTransit /* temp_file */)

#if defined(OS_MACOSX)
// Tells the utility process to analyze a DMG file for malicious download
// protection.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_AnalyzeDmgFileForDownloadProtection,
                     IPC::PlatformFileForTransit /* dmg_file */)
#endif  // defined(OS_MACOSX)
#endif  // defined(FULL_SAFE_BROWSING)

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

// Reply when the utility process has failed while unpacking and parsing a
// web resource.  |error_message| is a user-readable explanation of what
// went wrong.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackWebResource_Failed,
                     std::string /* error_message, if any */)

// Reply when the utility process has succeeded in decoding the image.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_DecodeImage_Succeeded,
                     SkBitmap /* decoded image */,
                     int /* delegate id */)

// Reply when an error occurred decoding the image.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_DecodeImage_Failed,
                     int /* delegate id */)

// Reply when a file has been patched.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_PatchFile_Finished, int /* result */)

#if defined(OS_CHROMEOS)
// Reply when the utility process has succeeded in creating the zip file.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_CreateZipFile_Succeeded)

// Reply when an error occured in creating the zip file.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_CreateZipFile_Failed)
#endif  // defined(OS_CHROMEOS)

// Reply when the utility process has started.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_ProcessStarted)

#if defined(FULL_SAFE_BROWSING)
// Reply when a zip file has been analyzed for malicious download protection.
IPC_MESSAGE_CONTROL1(
    ChromeUtilityHostMsg_AnalyzeZipFileForDownloadProtection_Finished,
    safe_browsing::zip_analyzer::Results)

#if defined(OS_MACOSX)
// Reply when a DMG file has been analyzed for malicious download protection.
IPC_MESSAGE_CONTROL1(
    ChromeUtilityHostMsg_AnalyzeDmgFileForDownloadProtection_Finished,
    safe_browsing::zip_analyzer::Results)
#endif  // defined(OS_MACOSX)
#endif  // defined(FULL_SAFE_BROWSING)

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
