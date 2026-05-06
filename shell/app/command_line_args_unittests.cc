// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#define U(x) L##x
#else
#define U(x) x
#endif

namespace electron {

namespace {

using StringViewType = base::CommandLine::StringViewType;
using StringVector = base::CommandLine::StringVector;

}  // namespace

// === IsUrlArg: real URI schemes must classify as URL =======================
// These cases are load-bearing for the CVE-2018-1000006 mitigation. Anything
// in this set must remain classified as a URL so that following positional
// args trip CheckCommandLineArguments.

TEST(CommandLineArgsTest, IsUrlArgRealSchemes) {
  EXPECT_TRUE(internal::IsUrlArg(U("https://example.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("http://example.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("HTTPS://example.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("sFtP://server")));
  EXPECT_TRUE(internal::IsUrlArg(U("mailto:user@example.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("ftp://files.example.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("electron-fiddle://gist/abc")));
  EXPECT_TRUE(internal::IsUrlArg(U("git+ssh://repo")));
  EXPECT_TRUE(internal::IsUrlArg(U("svn+ssh://repo")));
  EXPECT_TRUE(internal::IsUrlArg(U("x-com.app+sub:foo")));
  EXPECT_TRUE(internal::IsUrlArg(U("data:text/plain,Hi")));
  EXPECT_TRUE(internal::IsUrlArg(U("urn:isbn:0451450523")));
  EXPECT_TRUE(internal::IsUrlArg(U("file:///C:/path")));
  EXPECT_TRUE(internal::IsUrlArg(U("tel:+15551234567")));
  EXPECT_TRUE(internal::IsUrlArg(U("view-source:https://x.com")));
  EXPECT_TRUE(internal::IsUrlArg(U("chrome-extension://abcd")));
  EXPECT_TRUE(internal::IsUrlArg(U("ms-help://x")));
  EXPECT_TRUE(internal::IsUrlArg(U("iris.beep://server")));
  EXPECT_TRUE(internal::IsUrlArg(U("microsoft.windows.camera://action")));
  // Synthetic CVE attack scheme — must remain URL-classified.
  EXPECT_TRUE(internal::IsUrlArg(U("exodus://anything")));
  // Boundary: 2-char alphanumeric scheme.
  EXPECT_TRUE(internal::IsUrlArg(U("a1:rest")));
}

// === IsUrlArg: false positives the previous impl had =======================
// These were classified as URLs before the RFC 3986 tightening. Reclassifying
// them as non-URLs unblocks SSH/scp/mosh-style invocations on Windows.

TEST(CommandLineArgsTest, IsUrlArgRejectsSshStylePositionals) {
  EXPECT_FALSE(internal::IsUrlArg(U("user@host:22")));
  EXPECT_FALSE(internal::IsUrlArg(U("u@h:22")));
  EXPECT_FALSE(
      internal::IsUrlArg(U("very.long.user@some-host.example.com:22")));
  EXPECT_FALSE(internal::IsUrlArg(U("git@github.com:user/repo")));
}

TEST(CommandLineArgsTest, IsUrlArgRejectsWindowsPathValues) {
  EXPECT_FALSE(internal::IsUrlArg(U("key=value:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("C:\\path")));
  EXPECT_FALSE(internal::IsUrlArg(U("c:foo")));
}

TEST(CommandLineArgsTest, IsUrlArgRejectsNonSchemePunctuation) {
  EXPECT_FALSE(internal::IsUrlArg(U("ab/cd:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("ab\\cd:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("ab|cd:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a(b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a*b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a&b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a#b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a%b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a~b:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("a$b:foo")));
}

// === IsUrlArg: scheme grammar ==============================================

TEST(CommandLineArgsTest, IsUrlArgRequiresAlphaFirstChar) {
  EXPECT_FALSE(internal::IsUrlArg(U("+a:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("-a:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U(".a:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("1a:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("9zz:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U(" a:foo")));
  EXPECT_FALSE(internal::IsUrlArg(U("\ta:foo")));
}

TEST(CommandLineArgsTest, IsUrlArgAcceptsSchemeTailGrammar) {
  EXPECT_TRUE(internal::IsUrlArg(U("ab:rest")));
  EXPECT_TRUE(internal::IsUrlArg(U("a1:rest")));
  EXPECT_TRUE(internal::IsUrlArg(U("a+:rest")));
  EXPECT_TRUE(internal::IsUrlArg(U("a-:rest")));
  EXPECT_TRUE(internal::IsUrlArg(U("a.:rest")));
  EXPECT_TRUE(internal::IsUrlArg(U("abc.def+ghi-jkl0:rest")));
}

TEST(CommandLineArgsTest, IsUrlArgRejectsInvalidSchemeTailChars) {
  EXPECT_FALSE(internal::IsUrlArg(U("a_b:rest")));
  EXPECT_FALSE(internal::IsUrlArg(U("a/b:rest")));
  EXPECT_FALSE(internal::IsUrlArg(U("a b:rest")));
}

TEST(CommandLineArgsTest, IsUrlArgRejectsEmptyAndShortSchemes) {
  EXPECT_FALSE(internal::IsUrlArg(U("")));
  EXPECT_FALSE(internal::IsUrlArg(U(":")));
  EXPECT_FALSE(internal::IsUrlArg(U(":rest")));
  EXPECT_FALSE(internal::IsUrlArg(U("a:")));
  EXPECT_FALSE(internal::IsUrlArg(U("C:")));
  EXPECT_FALSE(internal::IsUrlArg(U("C:\\")));
  EXPECT_FALSE(internal::IsUrlArg(U("plain-arg-no-colon")));
  EXPECT_FALSE(internal::IsUrlArg(U("::")));
  EXPECT_FALSE(internal::IsUrlArg(U(":::")));
}

// === IsUrlArg: host:port disambiguation ====================================

TEST(CommandLineArgsTest, IsUrlArgDottedSchemeWithPortIsHostPort) {
  EXPECT_FALSE(internal::IsUrlArg(U("host.example.com:22")));
  EXPECT_FALSE(internal::IsUrlArg(U("myserver.lan:22")));
  EXPECT_FALSE(internal::IsUrlArg(U("a.b:80")));
  EXPECT_FALSE(internal::IsUrlArg(U("a.b:1")));
  EXPECT_FALSE(internal::IsUrlArg(U("a.b:65535")));
}

TEST(CommandLineArgsTest, IsUrlArgDottedSchemeBoundaryStaysUrl) {
  // Below valid port range.
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:0")));
  // Above valid port range.
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:65536")));
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:99999")));
  // Suffix too long.
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:100000")));
  // Empty suffix.
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:")));
  // Non-numeric suffix.
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:foo")));
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:22x")));
  EXPECT_TRUE(internal::IsUrlArg(U("a.b:22/path")));
}

TEST(CommandLineArgsTest, IsUrlArgUndottedSchemeWithDigitsStaysUrl) {
  // Heuristic only fires for dotted schemes.
  EXPECT_TRUE(internal::IsUrlArg(U("tel:22")));
  EXPECT_TRUE(internal::IsUrlArg(U("https:22")));
  EXPECT_TRUE(internal::IsUrlArg(U("a1:rest")));
}

// === CheckCommandLineArguments: CVE-2018-1000006 mitigation ===============

TEST(CommandLineArgsTest, CheckCommandLineArgumentsAllowsSafeArgv) {
  EXPECT_TRUE(CheckCommandLineArguments({}));
  EXPECT_TRUE(CheckCommandLineArguments({U("/path/to/electron")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("electron"), U("--no-sandbox"), U("--enable-logging")}));
  EXPECT_TRUE(CheckCommandLineArguments({U("electron"), U("https://x.com")}));
}

TEST(CommandLineArgsTest, CheckCommandLineArgumentsBlocksCveAttack) {
  // The original CVE-2018-1000006 exploit pattern.
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("exodus://aaaaa"), U("--gpu-launcher=cmd")}));
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("https://x.com/path"), U("--gpu-launcher=cmd")}));
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("electron-fiddle://abc"), U("--no-sandbox")}));
  // URL followed by plain positional, no flag.
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("https://x.com"), U("injected")}));
  // Compound RFC schemes must still trip the block.
  EXPECT_FALSE(
      CheckCommandLineArguments({U("prog"), U("git+ssh://x"), U("--inject")}));
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("x-com.app+sub://x"), U("--inject")}));
  EXPECT_FALSE(
      CheckCommandLineArguments({U("prog"), U("a.b.c://x"), U("--inject")}));
}

TEST(CommandLineArgsTest, CheckCommandLineArgumentsAllowsSshStyleInvocation) {
  // The bug this PR fixes — these were rejected pre-patch.
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("user@host:22"), U("--identity"), U("FOO")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("ssh"), U("user@host:22"), U("--identity"), U("FOO")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("mosh"), U("user@host:2201"), U("--identity"), U("FOO")}));
  EXPECT_TRUE(
      CheckCommandLineArguments({U("prog"), U("ssh"), U("host.example.com:22"),
                                 U("--identity"), U("FOO")}));
  EXPECT_TRUE(CheckCommandLineArguments({U("prog"), U("--no-sandbox"),
                                         U("user@host:22"), U("--identity"),
                                         U("FOO")}));
}

TEST(CommandLineArgsTest, CheckCommandLineArgumentsAllowsWindowsPathValues) {
  // #28595 example.
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("key=C:\\Path"), U("other-arg")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("C:\\Users\\me\\file.txt"), U("--some-flag")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("--open"), U("C:\\Users\\me\\file.txt")}));
}

TEST(CommandLineArgsTest, CheckCommandLineArgumentsHonorsDashDashSeparator) {
  // Args after `--` are unconditionally allowed.
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("--"), U("https://x"), U("--gpu-launcher=cmd")}));
  EXPECT_TRUE(CheckCommandLineArguments(
      {U("prog"), U("https://x"), U("--"), U("--gpu-launcher=cmd")}));
  // `--` reached AFTER a bad pattern doesn't help.
  EXPECT_FALSE(CheckCommandLineArguments(
      {U("prog"), U("https://x"), U("--gpu-launcher=cmd"), U("--")}));
}

}  // namespace electron
