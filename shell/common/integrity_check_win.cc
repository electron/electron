#include <Windows.h>
#include <Winuser.h>
#include <bcrypt.h>
#include <wincrypt.h>

#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>

#include "base/logging.h"

/*
External dependencies: Bcrypt.lib, Crypt32.lib, User32.lib

#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "User32.lib")
*/

using namespace std;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#define FILE_STREAM_READ_CHUNK_BYTES  1024 * 1024 * 32 // only load 32MB of data from stream per chunk
#define INTEGRITY_CHECK_RESOURCE_TYPE L"Integrity"
#define INTEGRITY_CHECK_RESOURCE_ITEM L"Checksums"

struct ResourceChecksum {
  string resource;
  string hash_alg;
  string hash_value;

  ResourceChecksum(const string& resource,
    const string& hash_alg,
    const string& hash_value)
    : resource(resource), hash_alg(hash_alg), hash_value(hash_value) {}

  wstring GetHashAlgWstr() const {
    return wstring(hash_alg.begin(), hash_alg.end());
  }
};

// Supported algorithms: https://learn.microsoft.com/en-us/windows/win32/seccng/cng-algorithm-identifiers
string GetInputStreamHash(istream& instream, const wstring& alg) {
  DWORD cbData = 0;
  NTSTATUS status = STATUS_UNSUCCESSFUL;

  BCRYPT_ALG_HANDLE hAlg = NULL;              // Bcrypt algorithm handle
  BCRYPT_HASH_HANDLE hHash = NULL;            // Bcrypt hash object handle

  PBYTE pbHashObject = NULL;                  // Bcrypt hash object pointer
  PBYTE pbHash = NULL;                        // Bcrypt hash result buffer pointer  
  PBYTE pbRaw = NULL;                         // To be hashed plaintext buffer

  DWORD cbHashObject = 0;                     // Bcrypt hash object size
  DWORD cbHash = 0;                           // Bcrypt hash result buffer size  
  DWORD cbRaw = FILE_STREAM_READ_CHUNK_BYTES; // Plaintext buffer size

  // Using shared defer pointer to release all resources when function returns
  shared_ptr<void> hash_resources_defer(nullptr, [&](...) {
    LOG(INFO) << " " << __func__ << "(): releasing Windows hash API resources";

    hAlg ?          BCryptCloseAlgorithmProvider(hAlg, 0) : 0;
    hHash ?         BCryptDestroyHash(hHash) : 0;
    pbHashObject ?  HeapFree(GetProcessHeap(), 0, pbHashObject) : true;
    pbHash ?        HeapFree(GetProcessHeap(), 0, pbHash) : true;
    pbRaw ?         HeapFree(GetProcessHeap(), 0, pbRaw) : true;
  });

  // Open an algoirthm handle
  if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hAlg, alg.data(), NULL, 0))) {
    LOG(WARNING) << " " << __func__ << "(): BCryptOpenAlgorithmProvider failed. Last error: " << GetLastError();
    return "";
  }

  // Calculate the size of hash object
  if (!NT_SUCCESS(status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0))) {
    LOG(WARNING) << " " << __func__ << "(): BCryptGetProperty failed. Last error: " << GetLastError();
    return "";
  }

  // Allocate the hash object on the heap
  pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
  if (!pbHashObject) {
    LOG(WARNING) << " " << __func__ << "(): HeapAlloc failed. Last error: " << GetLastError();
    return "";
  }

  // Calculate the size of hash result buffer
  if (!NT_SUCCESS(status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0))) {
    LOG(WARNING) << " " << __func__ << "(): BCryptGetProperty failed. Last error: " << GetLastError();
    return "";
  }

  // Allocate the hash result buffer on the heap
  pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHash);
  if (!pbHash) {
    LOG(WARNING) << " " << __func__ << "(): HeapAlloc failed. Last error: " << GetLastError();
    return "";
  }

  // Create Bcrypt hash object
  if (!NT_SUCCESS(status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0))) {
    LOG(WARNING) << " " << __func__ << "(): BCryptCreateHash failed. Last error: " << GetLastError();
    return "";
  }

  // Hash the data
  pbRaw = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbRaw);
  if (!pbRaw) {
    LOG(WARNING) << " " << __func__ << "(): HeapAlloc failed. Last error: " << GetLastError();
    return "";
  }

  streamsize total_size = 0;

  /*
    Reading the plaintext stream in chunks and each chunk size no larger than `cbRaw` size
    to avoid allocating too much memory to hash large files.
  */
  while (!instream.eof()) {
    LOG(INFO) << " " << __func__ << "(): start reading from stream";
    instream.read((char*)pbRaw, cbRaw);
    streamsize size = instream.gcount();
    total_size += size;
    LOG(INFO) << " " << __func__ << "(): finished reading " << to_string(size) << " bytes from stream";

    if (!NT_SUCCESS(
      status = BCryptHashData(hHash, pbRaw, size, 0))) {
      LOG(WARNING) << " " << __func__ << "(): BCryptHashData failed. Last error: " << GetLastError();
      return "";
    }
  }

  LOG(INFO) << " " << __func__ << "(): read total " << to_string(total_size) << " bytes from stream";

  // Finish the Bcrypt hash
  if (!NT_SUCCESS(status = BCryptFinishHash(hHash, pbHash, cbHash, 0))) {
    LOG(WARNING) << " " << __func__ << "(): BCryptFinishHash failed. Last error: " << GetLastError();
    return "";
  }

  // Hex encode the hash result
  DWORD hex_buffer_cb = 1024;
  char hex_buffer[1024];
  if (!CryptBinaryToStringA((BYTE*) pbHash, cbHash, CRYPT_STRING_HEXRAW | CRYPT_STRING_NOCRLF, hex_buffer, &hex_buffer_cb)) {
    LOG(WARNING) << " " << __func__ << "(): CryptBinaryToStringA failed. Last error: " << GetLastError();
    return "";
  }

  auto hex_encoded_hash = string(hex_buffer, hex_buffer_cb);
  for (char& c : hex_encoded_hash) {
    c = toupper(c);
  }

  return hex_encoded_hash;
}

vector<ResourceChecksum> LoadExpectedResourceChecksums() {
  HMODULE handle = ::GetModuleHandle(NULL);
  HRSRC rc = ::FindResource(handle, INTEGRITY_CHECK_RESOURCE_ITEM, INTEGRITY_CHECK_RESOURCE_TYPE);
  if (!rc) {
    LOG(WARNING) << " " << __func__ << "(): FindResource failed. Last error: " << GetLastError();
    return {};
  }

  HGLOBAL rcData = ::LoadResource(handle, rc);
  if (!rcData) {
    LOG(WARNING) << " " << __func__ << "(): LoadResource failed. Last error: " << GetLastError();
    return {};
  }

  auto* data = static_cast<const char*>(::LockResource(rcData));
  istringstream ss(data);

  vector<ResourceChecksum> resources;

  auto trimLineEndingCR = [](string& line) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
  };

  while (!ss.eof()) {
    string filepath, hash_alg, hash_value;
    if (getline(ss, filepath) && getline(ss, hash_alg) && getline(ss, hash_value)) {
      trimLineEndingCR(filepath);
      trimLineEndingCR(hash_alg);
      trimLineEndingCR(hash_value);

      for (auto& c : hash_value) {
        c = toupper(c);
      }

      resources.push_back({ filepath, hash_alg, hash_value });
    }
    else {
      break;
    }
  }

  return resources;
}

void TerminateProcessDueToIntegrityFailure() {
  LOG(ERROR) << " " << __func__ << "(): terminate process due to integrity check failure";
  MessageBox(NULL, L"One or more file is corrupted, please reinstall the application.", L"Error", MB_OK | MB_ICONERROR);
  exit(1);
}

void IntegrityCheck() {
  LOG(INFO) << " " << __func__ << "(): Started";
  LOG(INFO) << " " << __func__ << "(): process id: " << GetCurrentProcessId();

  // In case user enabled long paths on Windows, use 1024 instead of MAX_PATH to provide more flexibility.
  wchar_t buffer[1024];
  GetCurrentDirectoryW(1024, buffer);
  LOG(INFO) << " " << __func__ << "(): Current working directory: " << buffer;

  auto expected_checksums = LoadExpectedResourceChecksums();
  if (expected_checksums.empty()) {
    LOG(INFO) << " " << __func__ << "(): no expected checksums loaded. Skip resource integrity check.";
  }

  for (const auto& expected_checksum : expected_checksums) {
    LOG(INFO) << " " << __func__ << "(): Starting integrity check on resource: " << expected_checksum.resource;

    ifstream resource_file(expected_checksum.resource, ifstream::binary);
    string computed_hash = "";
    if (resource_file.is_open()) {
      computed_hash = GetInputStreamHash(resource_file, expected_checksum.GetHashAlgWstr());
      resource_file.close();
    } else {
      LOG(WARNING) << " " << __func__ << "(): Failed to open resource file: " << expected_checksum.resource.data();
    }

    LOG(INFO) << " " << __func__ << "(): Integrity check on resource: " << expected_checksum.resource;
    LOG(INFO) << " " << __func__ << "(): Expected hash: " << expected_checksum.hash_value;
    LOG(INFO) << " " << __func__ << "(): Computed hash: " << computed_hash;
    LOG(INFO) << " " << __func__ << "(): Hash match? " << (expected_checksum.hash_value == computed_hash ? "YES" : "NO");

    if (expected_checksum.hash_value != computed_hash) {
      TerminateProcessDueToIntegrityFailure();
    }    
  }

  LOG(INFO) << " " << __func__ << "(): Finished";
}
