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

UploadStream::Allocation UploadStream::Allocate(size_t size, uint64 syncPoint)
{
    return AllocateAligned(size, 1, syncPoint);
}

UploadStream::Allocation UploadStream::AllocateAligned(size_t size, size_t align, uint64 syncPoint)
{
    Page& page = m_pages.back();
    Allocation alloc;
    if (!page.Allocate(size, align, syncPoint, alloc))
    {
        page = AllocatePage();
        assert(page.Allocate(size, align, syncPoint, alloc));
    }
    return alloc;
}

void UploadStream::ResetAllocations(uint64 syncPoint)
{
    // Find the last page with page.syncPoint <= syncPoint, reset begin -> firstNotReset - 1 and stick them at the end of the list
    auto firstNotReset = m_pages.begin();
    for (firstNotReset; firstNotReset != m_pages.end(); firstNotReset++)
    {
        if (firstNotReset->GetSyncPoint() > syncPoint || firstNotReset->IsEmpty())
        {
            break;
        }
        else
        {
            firstNotReset->Reset();
        }
    }

    if (firstNotReset == m_pages.begin())
    {
        // First element has a higher syncPoint, syncPoint is monotonically increasing so no later elements are resetable either
        return;
    }

    m_pages.splice(m_pages.end(), m_pages, m_pages.begin(), firstNotReset);
}

UploadStream::Page::Page(size_t _pageSize, ID3D12Resource* _pageBuffer)
{
    m_pageSize = _pageSize;
    m_offset = 0;
    m_syncPoint = 0;
    m_pageBuffer = ComPtr<ID3D12Resource>(_pageBuffer);

    D3D12_RANGE range = {0,0};
    m_pageBuffer->Map(0, &range, &m_cpuAddr);

}

UploadStream::Page::~Page()
{
    m_pageBuffer->Unmap(0, nullptr);
}

void UploadStream::Page::Reset()
{
    m_offset = 0;
    m_syncPoint = 0;
}

bool UploadStream::Page::Allocate(size_t size, size_t align, uint64 syncPoint, Allocation& allocOut)
{
    size_t alignedOffset = Utils::AlignUp(m_offset, align);
    size_t nextOffset = alignedOffset + size;
    if (nextOffset > m_pageSize)
    {
        return false;
    }
    allocOut.buffer = m_pageBuffer.Get();
    allocOut.cpuAddr = (void*)((INT8*)m_cpuAddr + alignedOffset);
    allocOut.bufferOffset = alignedOffset;
    m_offset = nextOffset;
    m_syncPoint = std::max(m_syncPoint, syncPoint);

    return true;
}