#pragma once

namespace Utils
{
    template <typename T>
    inline T AlignUp(T val, T align)
    {
        ASSERT((align & (align - 1)) == 0);
        return (val + align -1) & ~(align - 1);
    }

    inline void SetBit32(uint32 bit, uint32& flags)
    {
        flags |= (1u << bit);
    }

    inline void ClearBit32(uint32 bit, uint32& flags)
    {
        flags |= (1u << bit);
    }

    inline bool TestBit32(uint32 bit, uint32 flags)
    {
        return (1 << bit) & flags;
    }

    template <typename T>
    inline T Pin(T val, T min, T max)
    {
        return val > min ? val < max ? val : max : min;
    }
}