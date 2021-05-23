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
    ASSERT(m_numComittedDescriptors + m_lastStagedSlot + 1 < m_maxResidentDescriptors);
    ASSERT(m_lastStagedSlot >= 0);

    uint32 gpuStartOffset = m_numComittedDescriptors * m_descriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dest = { m_gpuPool->GetCPUDescriptorHandleForHeapStart().ptr + gpuStartOffset };

    uint32 nDescToCommit = m_lastStagedSlot + 1;
    m_pDevice->GetNativeDevice()->CopyDescriptorsSimple(nDescToCommit, dest, m_cpuPool->GetCPUDescriptorHandleForHeapStart(), m_type);
    m_numComittedDescriptors += nDescToCommit;
    m_lastStagedSlot = -1;
    
    return { m_gpuPool->GetGPUDescriptorHandleForHeapStart().ptr + gpuStartOffset };
}

void DescriptorPool::Reset(
    void)
{
    ASSERT(m_lastStagedSlot == -1);
    m_numComittedDescriptors = 0;
}