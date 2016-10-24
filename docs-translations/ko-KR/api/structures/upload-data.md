# UploadData Object

* `bytes` Buffer - 전송되는 내용.
* `file` String - 업로드되는 파일의 경로.
* `blobUUID` String - BLOB 데이터의 UUID. 데이터를 이용하려면
  [ses.getBlobData](../session.md#sesgetblobdataidentifier-callback) 메소드를
  사용하세요.
