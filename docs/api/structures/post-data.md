# PostData Object

* `type` String - One of the following:
  * `rawData` - The data is available as a `Buffer`, in the `rawData` field.
  * `file` - The object represents a file. The `filePath`, `offset`, `length`
    and `modificationTime` fields will be used to describe the file.
  * `blob` - The object represents a `Blob`. The `blobUUID` field will be used
    to describe the `Blob`.
* `bytes` String (optional) - The raw bytes of the post data in a `Buffer`.
  Required for the `rawData` type.
* `filePath` String (optional) - The path of the file being uploaded. Required
  for the `file` type.
* `blobUUID` String (optional) - The `UUID` of the `Blob` being uploaded.
  Required for the `blob` type.
* `offset` Integer (optional) - The offset from the beginning of the file being
  uploaded, in bytes. Only valid for `file` types.
* `length` Integer (optional) - The length of the file being uploaded, in bytes.
  If set to `-1`, the whole file will be uploaded. Only valid for `file` types.
* `modificationTime` Double (optional) - The modification time of the file
  represented by a double, which is the number of seconds since the `UNIX Epoch`
  (Jan 1, 1970). Only valid for `file` types.
