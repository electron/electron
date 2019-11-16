# PostData Object

* `type` String - One of the following:
  * `rawData` - The data is available as a `Buffer`, in the `rawData` field.
  * `file` - The object represents a file. The `filePath`, `offset`, `length`
    and `modificationTime` fields will be used to describe the file.
  * `blob` - The object represents a `Blob`. The `blobUUID` field will be used to
    describe the `Blob`.
* `bytes` String - The raw bytes of the post data in a `Buffer`.
* `filePath` String - The path of the file being uploaded.
* `offset` Integer - The offset from the beginning of the file being uploaded,
  in bytes.
* `length` Integer - The length of the file being uploaded, in bytes. If set to
  `-1`, the whole file will be uploaded.
* `modificationTime` Double - The modification time of the file represented by
  a double, which is the number of seconds since the `UNIX Epoch` (Jan 1, 1970).
* `blobUUID` String - The `UUID` of the `Blob` being uploaded.
