workflow "Check for Release Notes in Pull Requests" {
  on = "pull_request"
  resolves = "Clerk"
}

action "Clerk" {
  uses = "electron/clerk"
  secrets = ["GITHUB_TOKEN"]
}