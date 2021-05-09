#include "UploadStream.h"

UploadStream::UploadStream(Device* device, size_t pageSize)
{
    assert(device != nullptr);
    m_device = device;
    m_pageSize = pageSize;

    AllocatePage();
}

UploadStream::Page& UploadStream::AllocatePage()
{
    D3D12_HEAP_PROPERTIES props = {};
    props.Type = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    ID3D12Resource* buffer;
    m_device->CreateBuffer(props, m_pageSize, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, &buffer);

    m_pages.emplace_back(m_pageSize, buffer);
    return m_pages.back();
}

UploadStream::Allocation UploadStream::Allocate(size_t size)
{
    return AllocateAligned(size, 1);
}

UploadStream::Allocation UploadStream::AllocateAligned(size_t size, size_t align)
{
    Page& page = m_pages.back();
    Allocation alloc;
    if (!page.Allocate(size, align, alloc))
    {
        page = AllocatePage();
        assert(page.Allocate(size, align, alloc));
    }
    return alloc;
}

void UploadStream::ResetAllocations()
{
    for (Page& page : m_pages)
    {
        page.Reset();
    }
}

UploadStream::Page::Page(size_t _pageSize, ID3D12Resource* _pageBuffer)
{
    pageSize = _pageSize;
    offset = 0;
    pageBuffer = ComPtr<ID3D12Resource>(_pageBuffer);

    D3D12_RANGE range = {0,0};
    pageBuffer->Map(0, &range, &cpuAddr);
}

UploadStream::Page::~Page()
{
    pageBuffer->Unmap(0, nullptr);
}

void UploadStream::Page::Reset()
{
    offset = 0;
}

bool UploadStream::Page::Allocate(size_t size, size_t align, Allocation& allocOut)
{
    size_t alignedOffset = Utils::AlignUp(offset, align);
    size_t nextOffset = alignedOffset + size;
    if (nextOffset > pageSize)
    {
        return false;
    }
    allocOut.buffer = pageBuffer.Get();
    allocOut.cpuAddr = (void*)((INT8*)cpuAddr + alignedOffset);
    allocOut.bufferOffset = alignedOffset;
    offset = nextOffset;

    return true;
}