#!/usr/bin/env python

import os
import sys
import re


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DOCS_DIR = os.path.join(SOURCE_ROOT, 'docs')


def main():
  os.chdir(SOURCE_ROOT)

  filepaths = []
  try:
    for root, dirs, files in os.walk(DOCS_DIR):
      for file in files:
        if file.endswith('.md'):
          filepaths.append(os.path.join(root, file))
  except KeyboardInterrupt:
    print('Keyboard interruption. Please try again.')
    return
  except:
    print('Error: Could not read files in directories.')
    return

  totalBrokenLinks = 0
  for path in filepaths:
    totalBrokenLinks += getBrokenLinks(path)

  print('Parsed through ' + str(len(filepaths)) + ' files.')
  print('Found ' + str(totalBrokenLinks) + ' broken relative links.')


def getBrokenLinks(filepath):
  currentDir = os.path.dirname(filepath)
  brokenLinks = []

  try:
    file = open(filepath, 'r')
    lines = file.readlines()
  except KeyboardInterrupt:
    print('Keyboard interruption whle parsing. Please try again.')
  except:
    print('Error: Could not open file ', filepath)
  finally:
    file.close()

  regexLink = re.compile('\[(.*?)\]\((?P<links>(.*?))\)')
  links = []
  for line in lines:
    matchLinks = regexLink.search(line)
    if matchLinks:
      relativeLink = matchLinks.group('links')
      if not str(relativeLink).startswith('http'):
        links.append(relativeLink)

  for link in links:
    sections = link.split('#')
    if len(sections) > 1:
      if str(link).startswith('#'):
        if not checkSections(sections, link, lines, filepath):
          brokenLinks.append(link)
      else:
        tempFile = os.path.join(currentDir, sections[0])
        if os.path.isfile(tempFile):
          try:
            newFile = open(tempFile, 'r')
            newLines = newFile.readlines()
          except KeyboardInterrupt:
            print('Keyboard interruption whle parsing. Please try again.')
          except:
            print('Error: Could not open file ', filepath)
          finally:
            newFile.close()

          if not checkSections(sections, link, newLines, tempFile):
            brokenLinks.append(link)
        else:
          brokenLinks.append(link)

    else:
      if not os.path.isfile(os.path.join(currentDir, link)):
        brokenLinks.append(link)

  print_errors(filepath, brokenLinks)
  return len(brokenLinks)


def checkSections(sections, link, lines, path):
  sectionHeader = sections[1].replace('-', '')
  regexSectionTitle = re.compile('# (?P<header>.*)')
  for line in lines:
    matchHeader = regexSectionTitle.search(line)
    if matchHeader:
     matchHeader = filter(str.isalnum, str(matchHeader.group('header')))
     if matchHeader.lower() == sectionHeader:
      return True
  return False


def print_errors(filepath, brokenLink):
  if brokenLink:
    print "File Location: " + filepath
    for link in brokenLink:
      print "\tBroken links: " + link


if __name__ == '__main__':
  sys.exit(main())
