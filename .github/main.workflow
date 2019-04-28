workflow "Clerk" {
  on = "pull_request"
  resolves = "Check release notes"
}

action "Check release notes" {
  uses = "electron/clerk@master"
  secrets = ["GITHUB_TOKEN"]
}

workflow "Archaeologist" {
  on = "pull_request"
  resolves = "Check typescript definitions"
}

action "Check typescript definitions" {
  uses = "electron/archaeologist@actions"
  secrets = ["GITHUB_TOKEN"]
}
