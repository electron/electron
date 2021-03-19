// @ts-check

const globby = require('globby');
const path = require('path');
const fs = require('fs-extra');
const DOCS_PATH = 'docs';

/**
 *
 * @param {string} startPath
 * @returns {Promise<Map<string, string>>}
 */
const getMarkdownFiles = async (startPath) => {
  const filesPaths = await globby(path.posix.join(process.cwd(), startPath, '**/*.md'));

  const files = new Map();
  for (const filePath of filesPaths) {
    const content = await fs.readFile(filePath, 'utf-8');
    files.set(filePath, content);
  }

  return files;
};

/**
 * Generates a title using the file name.
 * This should be used as a last resort.
 * @param {string} filepath
 */
const titleFromPath = (filepath) => {
  const filename = path.basename(filepath);
  const title = filename
  // Other logic here
    .replace(/-/g, ' ');

  return title;
};

/**
 * Removes markdown links and quotes
 * @param {string} content
 */
const cleanUpMarkdown = (content) => {
  const mdLinkRegex = /\[(.*?)\][([].*?[)\]]/mgi;
  const groups = content.matchAll(mdLinkRegex);
  let cleanedUp = content;

  if (!groups) {
    return cleanedUp;
  }

  for (const group of groups) {
    const [match, replacement] = group;
    cleanedUp = cleanedUp.replace(match, replacement);
  }

  // Other link formats and inline codeblocks
  cleanedUp = cleanedUp
    .replace(/\[`/g, '')
    .replace(/`\]/g, '')
    .replace(/`/g, '')
    .trim();

  return cleanedUp;
};

/**
 * Returns the first paragraph of content (first
 * occurrence of `\n\n`) without
 * considering the headers
 * @param {string} content
 */
const descriptionFromContent = (content) => {
  const lines = content.split('\n');

  let description = '';
  let subHeader = false;
  let wasPreviousLineEmpty = false;

  for (const line of lines) {
    const trimmedLine = line.trim();

    // The content of structures is often only bullet lists and no general description
    if (trimmedLine.startsWith('#') || trimmedLine.startsWith('*')) {
      if (subHeader) {
        return cleanUpMarkdown(description.trim());
      } else {
        subHeader = true;
      }
    } else if (trimmedLine.length === 0) {
      if (description.length > 0) {
        return cleanUpMarkdown(description.trim());
      }
    } else {
      description += `${trimmedLine.replace(/^>/, '')} `;
      wasPreviousLineEmpty = false;
    }
  }

  return description;
};

/**
 *
 * @param {string} content
 * @param {string} filepath
 */
const addFrontMatter = (content, filepath) => {
  if (content.startsWith('---')) {
    return content;
  }

  // Some pages (under API mostly) start with ## instead of #
  const titleRegex = /^##?\s(.*)$/mi;
  const titleMatches = content.match(titleRegex);
  const title = titleMatches
    ? titleMatches[1].trim()
    : titleFromPath(filepath).trim();

  const description = descriptionFromContent(content);

  const mdWithFrontmatter =
        `---
title: "${title}"
description: "${description}"
---

${content}`;

  return mdWithFrontmatter;
};

/**
 * Automatically adds a frontmatter to all the markdown
 * files under `startPath` using the first heading as
 * title and paragraph as description.
 * @param {string} startPath
 */
const addFrontmatterToAllDocs = async (startPath) => {
  const files = await getMarkdownFiles(startPath);

  for (const [filepath, content] of files) {
    const newContent = addFrontMatter(content, filepath);

    await fs.writeFile(filepath, newContent, 'utf-8');
  }
};

/**
 * Checks that all markdown files under `startPath` contain
 * a valid frontmatter.
 * @param {string} startPath
 */
const checkFrontmatterInAllDocs = async (startPath) => {
  // This is the regex used by [markdownlint](https://www.npmjs.com/package/markdownlint#user-content-optionsfrontmatter)
  // which is used to validate the md files of this project
  const frontmatterRegex = /((^---\s*$[^]*?^---\s*$)|(^\+\+\+\s*$[^]*?^(\+\+\+|\.\.\.)\s*$)|(^\{\s*$[^]*?^\}\s*$))(\r\n|\r|\n|$)/m;
  const files = await getMarkdownFiles(startPath);
  const errors = new Set();

  for (const [filepath, content] of files) {
    if (!frontmatterRegex.test(content)) {
      errors.add(filepath);
    }
  }

  if (errors.size > 0) {
    console.error('The following markdown files do not have a frontmatter section:');
    for (const file of errors) {
      console.error(file);
    }

    process.exitCode = 1;
  } else {
    console.log('All markdown files have a frontmatter section 🙌');
  }
};

/**
 *
 * @param {string} action
 */
const start = async (action) => {
  const root = DOCS_PATH;
  switch (action) {
    case '--check': await checkFrontmatterInAllDocs(root); break;
    case '--add': await addFrontmatterToAllDocs(root); break;
    default: console.log('No options passed. Use "--check" to validate, "--add" to add frontmatter to all md files'); break;
  }
};

start(process.argv[2]);
