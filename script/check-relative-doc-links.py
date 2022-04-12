#!/usr/bin/env python3

from __future__ import print_function
import os
import sys
import re


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DOCS_DIR = os.path.join(SOURCE_ROOT, 'docs')


def main():
  os.chdir(SOURCE_ROOT)

  filepaths = []
  totalDirs = 0
  try:
    for root, dirs, files in os.walk(DOCS_DIR):
      totalDirs += len(dirs)
      for f in files:
        if f.endswith('.md'):
          filepaths.append(os.path.join(root, f))
  except KeyboardInterrupt:
    print('Keyboard interruption. Please try again.')
    return

  totalBrokenLinks = 0
  for path in filepaths:
    totalBrokenLinks += getBrokenLinks(path)

  print('Parsed through ' + str(len(filepaths)) +
        ' files within docs directory and its ' +
        str(totalDirs) + ' subdirectories.')
  print('Found ' + str(totalBrokenLinks) + ' broken relative links.')
  return totalBrokenLinks


def getBrokenLinks(filepath):
  currentDir = os.path.dirname(filepath)
  brokenLinks = []

  try:
    f = open(filepath, 'r', encoding="utf-8")
    lines = f.readlines()
  except KeyboardInterrupt:
    print('Keyboard interruption while parsing. Please try again.')
  finally:
    f.close()

  linkRegexLink = re.compile('\[(.*?)\]\((?P<link>(.*?))\)')
  referenceLinkRegex = re.compile(
      '^\s{0,3}\[.*?\]:\s*(?P<link>[^<\s]+|<[^<>\r\n]+>)'
  )
  links = []
  for line in lines:
    matchLinks = linkRegexLink.search(line)
    matchReferenceLinks = referenceLinkRegex.search(line)
    if matchLinks:
      relativeLink = matchLinks.group('link')
      if not str(relativeLink).startswith('http'):
        links.append(relativeLink)
    if matchReferenceLinks:
      referenceLink = matchReferenceLinks.group('link').strip('<>')
      if not str(referenceLink).startswith('http'):
        links.append(referenceLink)

  for link in links:
    sections = link.split('#')
    if len(sections) < 2:
      if not os.path.isfile(os.path.join(currentDir, link)):
        brokenLinks.append(link)
    elif str(link).startswith('#'):
      if not checkSections(sections, lines):
        brokenLinks.append(link)
    else:
      tempFile = os.path.join(currentDir, sections[0])
      if os.path.isfile(tempFile):
        try:
          newFile = open(tempFile, 'r', encoding="utf-8")
          newLines = newFile.readlines()
        except KeyboardInterrupt:
          print('Keyboard interruption while parsing. Please try again.')
        finally:
          newFile.close()

        if not checkSections(sections, newLines):
          brokenLinks.append(link)
      else:
        brokenLinks.append(link)


  print_errors(filepath, brokenLinks)
  return len(brokenLinks)


def checkSections(sections, lines):
  invalidCharsRegex = '[^A-Za-z0-9_ \-]'
  sectionHeader = sections[1]
  regexSectionTitle = re.compile('# (?P<header>.*)')
  for line in lines:
    matchHeader = regexSectionTitle.search(line)
    if matchHeader:
      # This does the following to slugify a header name:
      #  * Replace whitespace with dashes
      #  * Strip anything that's not alphanumeric or a dash
      #  * Anything quoted with backticks (`) is an exception and will
      #    not have underscores stripped
      matchHeader = str(matchHeader.group('header')).replace(' ', '-')
      matchHeader = ''.join(
        map(
          lambda match: re.sub(invalidCharsRegex, '', match[0])
          + re.sub(invalidCharsRegex + '|_', '', match[1]),
          re.findall('(`[^`]+`)|([^`]+)', matchHeader),
        )
      )
      if matchHeader.lower() == sectionHeader:
        return True
  return False


def print_errors(filepath, brokenLink):
  if brokenLink:
    print("File Location: " + filepath)
    for link in brokenLink:
      print("\tBroken links: " + link)


if __name__ == '__main__':
  sys.exit(main())
