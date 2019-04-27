workflow "Clerk" {
  on = "pull_request"
  resolves = "Check release notes"
}

action "Check release notes" {
  uses = "electron/clerk@master"
  secrets = [ "GITHUB_TOKEN" ]
}
