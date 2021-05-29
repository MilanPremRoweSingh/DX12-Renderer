#include "DescriptorPool.h"

#include "Device.h"

DescriptorPool::DescriptorPool(
    Device* pDevice,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32 maxResidentDescriptors,
    uint32 numSlots)
{
    m_pDevice = pDevice;
    m_type = type;
    m_maxSlots = numSlots;
    m_maxResidentDescriptors = maxResidentDescriptors;

    D3D12_DESCRIPTOR_HEAP_DESC gpuPoolDesc = {};
    gpuPoolDesc.NumDescriptors = maxResidentDescriptors;
    gpuPoolDesc.Type = m_type;
    gpuPoolDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    m_pDevice->CreateDescriptorHeap(gpuPoolDesc, &m_gpuPool, m_descriptorSize);

    D3D12_DESCRIPTOR_HEAP_DESC cpuPoolDesc = {};
    cpuPoolDesc.NumDescriptors = m_maxSlots;
    cpuPoolDesc.Type = m_type;
    cpuPoolDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_pDevice->CreateDescriptorHeap(cpuPoolDesc, &m_cpuPool);

    m_lastStagedSlot = -1;

    m_head = 0;
    m_currTail = 0;
}

void DescriptorPool::Reset(
    uint64 syncPoint)
{
    ASSERT(m_lastStagedSlot == -1);

    if (m_tails.empty())
    {
        return;
    }

    Tail& oldestTail = m_tails.front();
    while (oldestTail.syncPoint < syncPoint)
    {
        m_head = oldestTail.offset;
        m_tails.pop();
        if (m_tails.empty())
        {
            // If we've freed all previous frames, the pool should be empty
            ASSERT(m_head == m_currTail);
            break;
        }
        oldestTail = m_tails.front();
    }
}

void DescriptorPool::EndFrame(
    uint64 syncPoint)
{
    ASSERT(m_lastStagedSlot == -1);
    m_tails.emplace(m_currTail, syncPoint);
}


void DescriptorPool::StageDescriptor(
    int32 slot,
    D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE dest = { m_cpuPool->GetCPUDescriptorHandleForHeapStart().ptr + slot * m_descriptorSize };
    m_pDevice->GetNativeDevice()->CopyDescriptorsSimple(1, dest, handle, m_type);
    m_lastStagedSlot = std::max(slot, m_lastStagedSlot);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::CommitStagedDescriptors(
    void)
{
    ASSERT(m_lastStagedSlot >= 0);

    uint32 descriptorsToCommit = m_lastStagedSlot + 1;
    // Wrap back around if we've reached the end
    if (m_currTail + descriptorsToCommit >= m_maxResidentDescriptors)
    {
        ASSERT(m_head != 0);
        m_currTail = 0;
    }

    // Descriptor Pool out of memory
    ASSERT(m_currTail >= m_head || m_currTail + descriptorsToCommit < m_head);

    uint32 gpuStartOffset = m_currTail * m_descriptorSize;

    D3D12_CPU_DESCRIPTOR_HANDLE dest = { m_gpuPool->GetCPUDescriptorHandleForHeapStart().ptr + gpuStartOffset };

    m_pDevice->GetNativeDevice()->CopyDescriptorsSimple(descriptorsToCommit, dest, m_cpuPool->GetCPUDescriptorHandleForHeapStart(), m_type);
    m_currTail += descriptorsToCommit;
    m_lastStagedSlot = -1;
    
    return { m_gpuPool->GetGPUDescriptorHandleForHeapStart().ptr + gpuStartOffset };
}
