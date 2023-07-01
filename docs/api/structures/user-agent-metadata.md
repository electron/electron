# UserAgentMetadata Object

* `brands` [UserAgentBrandVersion[]](user-agent-brand-version.md) - The brand and versions.
* `fullVersionList` [UserAgentBrandVersion[]](user-agent-brand-version.md) - The brand and full versions.
* `fullVersion` string - full browser version.
* `platform` string - The platform.
* `platformVersion` string - The platform version.
* `model` string - The model of mobile device.
* `mobile` boolean - Whether the user agent is running on a mobile device.
* `architecture` string - The platform architecture.
* `bitness` string - The architecture bitness.
* `wow64` string - Whether the user agent is running on a Wow64 platform.

See the MDN documentation for
[User-Agent Client Hints API](https://developer.mozilla.org/en-US/docs/Web/API/User-Agent_Client_Hints_API)
and the W3C document for
[User-Agent Client Hints](https://wicg.github.io/ua-client-hints/#ref-for-dictdef-uadatavalues)
for more details.
