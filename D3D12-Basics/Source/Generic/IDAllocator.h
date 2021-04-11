#include "SimpleDequeue.h"

// Not thread safe! For now
class IDAllocator
{
public:
    int32 AllocID()
    {
        if (m_unusedIDs.IsEmpty())
        {
            return m_unusedIDs.PopBack();
        }

        int32 id = m_nextID++;
        ASSERT(m_nextID > 0);
        return m_nextID;
    }

    void FreeID(int32 id)
    {
        m_unusedIDs.PushBack(id);
    }

private:
    SimpleDequeue<int32> m_unusedIDs;
    int32 m_nextID = -1;
};