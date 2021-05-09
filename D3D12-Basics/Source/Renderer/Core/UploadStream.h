#pragma once

#include "Renderer/Core/Device.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <list>

using Microsoft::WRL::ComPtr;

// For now, this will just be a page based linear allocator
// Maybe it would be worth having pages which allocate linearly that are stored in a heap, but this will be fine to start (or that's too slow)
// Having a 'destruction queue' or something like that could be interesting
class UploadStream
{
public:
    struct Allocation
    {
        // Afaik you have to have a pointer to the buffer itself to be able to copy from it, so allocations must track the resource they came from
        ID3D12Resource* buffer;
        size_t bufferOffset;
        void* cpuAddr;

        inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress()
        {
            return buffer->GetGPUVirtualAddress() + bufferOffset;
        }
    };
    
    UploadStream(Device* device, size_t pageSize = _2MB);

    Allocation Allocate(size_t size);
    Allocation AllocateAligned(size_t size, size_t align);

    void ResetAllocations();

private:
    struct Page
    {
        ComPtr<ID3D12Resource> pageBuffer;
        void* cpuAddr;
        size_t offset;
        size_t pageSize;

        Page() = delete;
        Page(size_t _pageSize, ID3D12Resource* _pageBuffer);
        ~Page();

        bool Allocate(size_t size, size_t align, Allocation& allocOut);

        void Reset();
    };

    Device* m_device;

    Page& AllocatePage();

    std::list<Page> m_pages;
    size_t m_pageSize;
};

