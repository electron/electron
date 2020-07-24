{
  "targets": [
    {
      "target_name": "test_module",
      "sources": [ "main.cpp" ],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    },
    {
      "target_name": "libfoo",
      "type": "shared_library",
      "sources": [ "foo.cpp" ]
    }
  ]
}