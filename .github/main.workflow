workflow "Check for Release Notes in Pull Requests" {
  on = "pull_request"
  resolves = "Clerk"
}

action "Clerk" {
  uses = "electron/clerk@master"
  secrets = ["GITHUB_TOKEN"]
}