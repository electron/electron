#define NAPI_CREATE_STRING(str)                                  \
  [&]() {                                                        \
    napi_value value;                                            \
    napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &value); \
    return value;                                                \
  }()

#define NAPI_GET_PROPERTY_VALUE(obj, field)                         \
  [&]() {                                                           \
    napi_value value;                                               \
    napi_get_property(env, obj, NAPI_CREATE_STRING(field), &value); \
    return value;                                                   \
  }()

#define NAPI_GET_PROPERTY_VALUE_STRING(obj, field)                 \
  [&]() {                                                          \
    auto val = NAPI_GET_PROPERTY_VALUE(obj, field);                \
    size_t size;                                                   \
    napi_get_value_string_utf8(env, val, nullptr, 0, &size);       \
    char* buffer = new char[size + 1];                             \
    napi_get_value_string_utf8(env, val, buffer, size + 1, &size); \
    return std::string(buffer);                                    \
  }()

#define NAPI_GET_PROPERTY_VALUE_BUFFER(obj, field)                 \
  [&]() {                                                          \
    auto val = NAPI_GET_PROPERTY_VALUE(obj, field);                \
    size_t size;                                                   \
    napi_create_buffer(env, val, nullptr, 0, &size);       \
    char* buffer = new char[size + 1];                             \
    napi_get_value_string_utf8(env, val, buffer, size + 1, &size); \
    return std::string(buffer);                                    \
  }()