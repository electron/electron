# UploadData Object

* `contentType` String (optional) - Content type of the content to be sent.
* `bytes` Buffer - Content being sent.
* `file` String (optional) - Path of file being uploaded.
* `blobUUID` String (optional) - UUID of blob data. Use [ses.getBlobData](../session.md#sesgetblobdataidentifier) method
  to retrieve the data.
