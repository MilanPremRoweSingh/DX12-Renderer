#pragma once

namespace Utils
{
    template <typename T>
    T AlignUp(T val, T align)
    {
        assert((align & (align - 1)) == 0);
        return (val + align -1) & ~(align - 1);
    }
}