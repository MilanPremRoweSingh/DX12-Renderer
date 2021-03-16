#include "UploadStream.h"
#include "D3D12Context.h"


UploadStream::UploadStream(ID3D12Device* device, uint32 pageSize)
{
    assert(device != nullptr);
    m_device = device;

    m_pageSize = pageSize;
    m_pages.emplace_back();
}

UploadStream::Page::Page()
{
    
}