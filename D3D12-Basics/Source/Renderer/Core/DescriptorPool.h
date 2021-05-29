#pragma once
#include "D3D12Header.h"

#include <queue>

class Device;

// Naive descriptor pool, not set up for multi-frame synchronisation. Will improve on this later
class DescriptorPool
{
public:
    DescriptorPool(
        Device* pDevice,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32 maxResidentDescriptors,
        uint32 numSlots);

    void StageDescriptor(
        int32 slot,
        D3D12_CPU_DESCRIPTOR_HANDLE handle);

    D3D12_GPU_DESCRIPTOR_HANDLE CommitStagedDescriptors(
        void);

    void Reset(
        uint64 syncPoint);

    void EndFrame(
        uint64 syncPoint);

    inline ID3D12DescriptorHeap* GetGPUDescriptorHeap()
    {
        return m_gpuPool.Get();
    }


private:
    Device* m_pDevice;

    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    uint32 m_descriptorSize;
    uint32 m_maxSlots;
    uint32 m_maxResidentDescriptors;

    ComPtr<ID3D12DescriptorHeap> m_cpuPool;
    int32 m_lastStagedSlot;

    ComPtr<ID3D12DescriptorHeap> m_gpuPool;
    uint32 m_head;
    uint32 m_currTail;

    struct Tail
    {
        Tail(uint32 _offset, uint64 _syncPoint) : 
            offset(_offset),
            syncPoint(_syncPoint) {}

        uint32 offset;
        uint64 syncPoint;
    };

    std::queue<Tail> m_tails;
};

