# UserAgentMetadata Object

* `brands` [UserAgentBrandVersion[]](user-agent-brand-version.md) (optional) - The brand and versions.
* `fullVersionList` [UserAgentBrandVersion[]](user-agent-brand-version.md) (optional) - The brand and full versions.
* `fullVersion` string (optional) - full browser version.
* `platform` string (optional) - The platform.
* `platformVersion` string (optional) - The platform version.
* `model` string (optional) - The model of mobile device.
* `mobile` boolean (optional) - Whether the user agent is running on a mobile device.
* `architecture` string (optional) - The platform architecture.
* `bitness` string (optional) - The architecture bitness.
* `wow64` boolean (optional) - Whether the user agent is running on a Wow64 platform.
* `formFactors` string[] (optional) - The device form factors.

See the MDN documentation for
[User-Agent Client Hints API](https://developer.mozilla.org/en-US/docs/Web/API/User-Agent_Client_Hints_API)
and the W3C document for
[User-Agent Client Hints](https://wicg.github.io/ua-client-hints/#ref-for-dictdef-uadatavalues)
for more details.
