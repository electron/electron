#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <js_native_api.h>
#include <node_api.h>
#include <wrl/client.h>

#include <iostream>
#include <string>

#include "napi_utils.h"

namespace {

Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
Microsoft::WRL::ComPtr<ID3D11Device1> device1 = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;

UINT cached_width = 0;
UINT cached_height = 0;
Microsoft::WRL::ComPtr<ID3D11Texture2D> cached_staging_texture = nullptr;

napi_value ExtractPixels(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok)
    return nullptr;

  if (argc != 1) {
    napi_throw_error(env, nullptr,
                     "Wrong number of arguments, expected textureInfo");
  }

  auto textureInfo = args[0];

  auto widgetType = NAPI_GET_PROPERTY_VALUE_STRING(textureInfo, "widgetType");
  auto pixelFormat = NAPI_GET_PROPERTY_VALUE_STRING(textureInfo, "pixelFormat");
  auto sharedTextureHandle =
      NAPI_GET_PROPERTY_VALUE(textureInfo, "sharedTextureHandle");

  size_t handleBufferSize;
  uint8_t* handleBufferData;
  napi_get_buffer_info(env, sharedTextureHandle,
                       reinterpret_cast<void**>(&handleBufferData),
                       &handleBufferSize);

  auto handle = *reinterpret_cast<HANDLE*>(handleBufferData);
  std::cout << "ExtractPixels widgetType=" << widgetType
            << " pixelFormat=" << pixelFormat
            << " sharedTextureHandle=" << handle << std::endl;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> shared_texture = nullptr;
  HRESULT hr =
      device1->OpenSharedResource1(handle, IID_PPV_ARGS(&shared_texture));
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "Failed to open shared texture resource");
    return nullptr;
  }

  // Extract the texture description
  D3D11_TEXTURE2D_DESC desc;
  shared_texture->GetDesc(&desc);

  // Cache the staging texture if it does not exist or size has changed
  if (!cached_staging_texture || cached_width != desc.Width ||
      cached_height != desc.Height) {
    if (cached_staging_texture) {
      cached_staging_texture->Release();
    }

    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    std::cout << "Create staging Texture2D width=" << desc.Width
              << " height=" << desc.Height << std::endl;
    hr = device->CreateTexture2D(&desc, nullptr, &cached_staging_texture);
    if (FAILED(hr)) {
      napi_throw_error(env, "osr-gpu", "Failed to create staging texture");
      return nullptr;
    }

    cached_width = desc.Width;
    cached_height = desc.Height;
  }

  // Copy the shared texture to the staging texture
  context->CopyResource(cached_staging_texture.Get(), shared_texture.Get());

  // Calculate the size of the buffer needed to hold the pixel data
  // 4 bytes per pixel
  size_t bufferSize = desc.Width * desc.Height * 4;

  // Create a NAPI buffer to hold the pixel data
  napi_value result;
  void* resultData;
  status = napi_create_buffer(env, bufferSize, &resultData, &result);
  if (status != napi_ok) {
    napi_throw_error(env, "osr-gpu", "Failed to create buffer");
    return nullptr;
  }

  // Map the staging texture to read the pixel data
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  hr = context->Map(cached_staging_texture.Get(), 0, D3D11_MAP_READ, 0,
                    &mappedResource);
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "Failed to map the staging texture");
    return nullptr;
  }

  // Copy the pixel data from the mapped resource to the NAPI buffer
  const uint8_t* srcData = static_cast<const uint8_t*>(mappedResource.pData);
  uint8_t* destData = static_cast<uint8_t*>(resultData);
  for (UINT row = 0; row < desc.Height; ++row) {
    memcpy(destData + row * desc.Width * 4,
           srcData + row * mappedResource.RowPitch, desc.Width * 4);
  }

  // Unmap the staging texture
  context->Unmap(cached_staging_texture.Get(), 0);
  return result;
}

napi_value InitializeGpu(napi_env env, napi_callback_info info) {
  HRESULT hr;

  // Feature levels supported
  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
  UINT num_feature_levels = ARRAYSIZE(feature_levels);
  D3D_FEATURE_LEVEL feature_level;

  // This flag adds support for surfaces with a different color channel ordering
  // than the default. It is required for compatibility with Direct2D.
  UINT creation_flags =
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;

  // We need dxgi to share texture
  Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory = nullptr;
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter = nullptr;
  hr = CreateDXGIFactory(IID_IDXGIFactory2, (void**)&dxgi_factory);
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "CreateDXGIFactory failed");
    return nullptr;
  }

  hr = dxgi_factory->EnumAdapters(0, &adapter);
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "EnumAdapters failed");
    return nullptr;
  }

  DXGI_ADAPTER_DESC adapter_desc;
  adapter->GetDesc(&adapter_desc);
  std::wcout << "Initializing DirectX with adapter: "
             << adapter_desc.Description << std::endl;

  hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                         creation_flags, feature_levels, num_feature_levels,
                         D3D11_SDK_VERSION, &device, &feature_level, &context);
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "D3D11CreateDevice failed");
    return nullptr;
  }

  hr = device->QueryInterface(IID_PPV_ARGS(&device1));
  if (FAILED(hr)) {
    napi_throw_error(env, "osr-gpu", "Failed to open d3d11_1 device");
    return nullptr;
  }

  return nullptr;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {
      {"ExtractPixels", NULL, ExtractPixels, NULL, NULL, NULL, napi_default,
       NULL},
      {"InitializeGpu", NULL, InitializeGpu, NULL, NULL, NULL, napi_default,
       NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  std::cout << "Initialized osr-gpu native module" << std::endl;
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
