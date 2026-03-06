#!/usr/bin/env node

const chalk = require('chalk');

const { execSync } = require('node:child_process');

/**
 * Check if commits are GPG signed on push
 */
function checkSignedCommits () {
  try {
    if (process.stdin.isTTY) {
      // If called directly (not from git hook), check all commits for branch
      console.log(chalk.blue('Checking all commits in branch for signatures...'));
      checkBranchCommits();
      return;
    }

    // Get the commits that are being pushed
    const input = process.stdin;
    let stdinData = '';

    // Read from stdin when called by git hook
    input.on('data', (data) => {
      stdinData += data.toString();
    });

    input.on('end', () => {
      if (!stdinData.trim()) {
        console.log(chalk.blue('No commits to check.'));
        return;
      }

      const lines = stdinData.trim().split('\n');
      let hasUnsignedCommits = false;

      for (const line of lines) {
        const [, localSha, , remoteSha] = line.split(' ');

        // Skip if deleting a branch
        if (localSha === '0000000000000000000000000000000000000000') {
          continue;
        }

        hasUnsignedCommits = hasUnsignedCommits || checkCommitsForSha(localSha, remoteSha);
      }

      if (hasUnsignedCommits) {
        console.log(chalk.red('\n🚫 Push rejected: Some commits are not signed!'));
        console.log(chalk.yellow('See GitHub\'s documentation on telling Git about your signing key: https://docs.github.com/en/authentication/managing-commit-signature-verification/telling-git-about-your-signing-key'));
        process.exit(1);
      }

      console.log(chalk.green('\n✅ All commits are signed!'));
    });
  } catch (error) {
    console.error(chalk.red('Error in pre-push hook:', error.message));
    process.exit(1);
  }
}

function checkBranchCommits () {
  try {
    // Get the last commit
    const lastCommit = callGitCommand('rev-parse HEAD').trim().split('\n').filter(Boolean);

    const hasUnsignedCommits = checkCommitsForSha(lastCommit);

    if (hasUnsignedCommits) {
      console.log(chalk.yellow('\n⚠️  Some commits are not signed.'));
      console.log(chalk.yellow('See GitHub\'s documentation on telling Git about your signing key: https://docs.github.com/en/authentication/managing-commit-signature-verification/telling-git-about-your-signing-key'));
    } else {
      console.log(chalk.green('\n✅ All local commits are signed!'));
    }
  } catch (error) {
    console.error(chalk.red('Error checking recent commits:', error.message));
    process.exit(1);
  }
}

function isCommitSigned (commitSha) {
  try {
    // Check if commit has a GPG signature
    callGitCommand(`verify-commit ${commitSha}`);
    return true;
  } catch (error) {
    // Strip "Command failed: git verify-commit commitSha" from the error message and print the rest for more details on why the commit is unsigned
    const errorMessage = error.message.replace(`Command failed: git verify-commit ${commitSha}`, '').trim();
    if (errorMessage && errorMessage.length > 0) {
      console.error(chalk.red(`Commit ${commitSha.substring(0, 8)} is unsigned; additional details:`, errorMessage));
    }
    // git verify-commit returns non-zero exit code for unsigned commits
    return false;
  }
}

function checkCommitsForSha (localSha, remoteSha) {
  let hasUnsignedCommits = false;
  let commits = [];
  try {
    // Get the range of commits being pushed
    if (!remoteSha || remoteSha === '0000000000000000000000000000000000000000') {
      // Find all commits from merge base to tip
      try {
        // Try to find merge base with default branch
        let mergeBase;
        try {
          console.log(chalk.blue('Determining merge base for new branch...'));
          // First try to use the upstream branch if set
          mergeBase = callGitCommand('merge-base HEAD @{upstream}').trim();
        } catch {
          try {
            // Try to get the default branch from origin/HEAD
            mergeBase = callGitCommand('merge-base HEAD origin/HEAD').trim();
          } catch {
            try {
              // Try to detect the default branch dynamically
              const defaultBranch = callGitCommand('symbolic-ref refs/remotes/origin/HEAD')
                .trim().replace('refs/remotes/', '');
              mergeBase = callGitCommand(`merge-base HEAD ${defaultBranch}`).trim();
            } catch {
              console.log(chalk.yellow('Warning: Could not determine merge base for new branch, checking all commits in branch'));
              // If all else fails, check all commits in the branch
              // This might happen for orphaned branches
              commits = callGitCommand(`rev-list ${localSha}`).trim().split('\n').filter(Boolean);
            }
          }
        }

        if (mergeBase) {
          commits = callGitCommand(`rev-list ${mergeBase}..${localSha}`)
            .trim().split('\n').filter(Boolean);
        }
      } catch (error) {
        console.error(chalk.yellow(`Warning: Could not determine commit range for new branch, checking tip commit only: ${error.message}`));
        commits = [localSha];
      }
    } else {
      // Existing branch, check commits between remoteSha and localSha
      const range = `${remoteSha}..${localSha}`;
      commits = callGitCommand(`rev-list ${range}`).trim().split('\n').filter(Boolean);
    }

    for (const commit of commits) {
      if (!isCommitSigned(commit)) {
        console.log(chalk.red(`❌ Commit ${commit.substring(0, 8)} is not signed!`));
        hasUnsignedCommits = true;
      } else {
        console.log(chalk.green(`✅ Commit ${commit.substring(0, 8)} is signed`));
      }
    }
    return hasUnsignedCommits;
  } catch (error) {
    console.error(chalk.red('Error checking commits:', error.message));
    process.exit(1);
  }
}

function callGitCommand (command) {
  return execSync(`git ${command}`, { encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] });
}

// Run the check
checkSignedCommits();
