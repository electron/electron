{
    "targets": [
        {
            "target_name": "osr-gpu",
            "sources": ['napi_utils.h'],
            "conditions": [
                ['OS=="win"', {
                    'sources': ['binding_win.cc'],
                    'link_settings': {
                        'libraries': ['dxgi.lib', 'd3d11.lib', 'dxguid.lib'],
                    }
                }],
            ],
        }
    ]
}
