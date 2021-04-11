#include "SimpleDequeue.h"
#include <type_traits>

// Not thread safe! MUST use an enum with an explicitly defined type!
template <typename IDEnumType>
class IDAllocator
{
public:
    IDAllocator(IDEnumType initialID) :
        m_nextID(initialID) {}

    IDEnumType AllocID()
    {
        if (m_unusedIDs.IsEmpty())
        {
            return m_unusedIDs.PopBack();
        }

        IDEnumType id = m_nextID;
        // This is kinda  gross, but I want type safety for the returned IDs 
        m_nextID = (IDEnumType)((__underlying_type(IDEnumType))m_nextID + 1);
        ASSERT(m_nextID > 0);
        return id;
    }

    void FreeID(IDEnumType id)
    {
        m_unusedIDs.PushBack(id);
    }

private:
    SimpleDequeue<IDEnumType> m_unusedIDs;
    IDEnumType m_nextID;
};