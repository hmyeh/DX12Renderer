#include "dx12_api.h"

#include <cassert>

// Directx headers
#include <dxgi1_6.h>


#include "utility.h"

namespace directx
{

    void EnableDebugLayer()
    {
#if defined(_DEBUG)
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
        debug_interface->EnableDebugLayer();
#endif
    }


    Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool use_warp)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
        UINT create_factory_flags = 0;
#if defined(_DEBUG)
        create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

        Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter1;
        Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter4;

        if (use_warp)
        {
            ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter1)));
            ThrowIfFailed(dxgi_adapter1.As(&dxgi_adapter4));
        }
        else
        {
            SIZE_T max_dedicated_video_memory = 0;
            for (UINT i = 0; dxgi_factory->EnumAdapters1(i, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 dxgi_adapter_desc1;
                dxgi_adapter1->GetDesc1(&dxgi_adapter_desc1);

                // Check to see if the adapter can create a D3D12 device without actually 
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ((dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                    SUCCEEDED(D3D12CreateDevice(dxgi_adapter1.Get(),
                        D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                    dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory)
                {
                    max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
                    ThrowIfFailed(dxgi_adapter1.As(&dxgi_adapter4));
                }
            }
        }

        return dxgi_adapter4;
    }


    Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
    {
        Microsoft::WRL::ComPtr<ID3D12Device2> d3d12_device2;
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device2)));

        // Enable debug messages in debug mode.
#if defined(_DEBUG)
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue;
        if (SUCCEEDED(d3d12_device2.As(&info_queue)))
        {
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID deny_ids[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            filter.DenyList.NumIDs = _countof(deny_ids);
            filter.DenyList.pIDList = deny_ids;

            ThrowIfFailed(info_queue->PushStorageFilter(&filter));
        }
#endif

        return d3d12_device2;
    }


    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12_command_queue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12_command_queue)));

        return d3d12_command_queue;
    }

    bool CheckTearingSupport()
    {
        BOOL allow_tearing = FALSE;

        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
        // graphics debugging tools which will not support the 1.5 factory interface 
        // until a future update.
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
        {
            Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
            if (SUCCEEDED(factory4.As(&factory5)))
            {
                if (FAILED(factory5->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                    &allow_tearing, sizeof(allow_tearing))))
                {
                    allow_tearing = FALSE;
                }
            }
        }

        return allow_tearing == TRUE;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue, uint32_t width, uint32_t height, uint32_t buffer_count)
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgi_swapchain4;
        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory4;
        UINT create_factory_flags = 0;
#if defined(_DEBUG)
        create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory4)));

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.Width = width;
        swap_chain_desc.Height = height;
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.Stereo = FALSE;
        swap_chain_desc.SampleDesc = { 1, 0 };
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = buffer_count;
        swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swap_chain_desc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1;
        ThrowIfFailed(dxgi_factory4->CreateSwapChainForHwnd(
            command_queue.Get(),
            hWnd,
            &swap_chain_desc,
            nullptr,
            nullptr,
            &swap_chain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgi_factory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swap_chain1.As(&dxgi_swapchain4));

        return dxgi_swapchain4;
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num_descriptors)
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = num_descriptors;
        desc.Type = type;

        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap)));

        return descriptor_heap;
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
        ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator)));
        return command_allocator;
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
        ThrowIfFailed(device->CreateCommandList(0, type, command_allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        return commandList;
    }

    Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(Microsoft::WRL::ComPtr<ID3D12Device2> device)
    {
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;

        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return fence;
    }

    HANDLE CreateEventHandle()
    {
        HANDLE fence_event;

        fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fence_event && "Failed to create fence event.");

        return fence_event;
    }
}
