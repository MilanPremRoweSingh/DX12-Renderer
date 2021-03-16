#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <list>

using Microsoft::WRL::ComPtr;

// For now, this will just be a page based linear allocator
// Maybe it would be worth having pages which allocate linearly that are stored in a heap, but this will be fine to start
class UploadStream
{
    UploadStream(ID3D12Device* device, uint32 pageSize = _2MB);
    

private:
    struct Page
    {
        ComPtr<ID3D12Resource> m_pageBuffer;
        uint32 m_offset;
        uint32 m_size;

        Page();
    };

    ID3D12Device* m_device;

    std::list<Page> m_pages;
    uint32 m_pageSize;

};

