workflow "Clerk" {
  #TODO(codebytere): make this work properly on pull_request
  on = "repository_dispatch"
  resolves = "Check release notes"
}

action "Check release notes" {
  uses = "electron/clerk@master"
  secrets = [ "GITHUB_TOKEN" ]
}
