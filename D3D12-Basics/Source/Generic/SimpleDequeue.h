#pragma once


template <typename T>
class SimpleDequeue
{
public:
    void PushFront(T value)
    {
        SimpleDequeueNode* node = new SimpleDequeueNode(value);
        if (!tail)
        {
            head = tail = node;
        }
        else
        {
            node->next = head;
            head->prev = node;
            head = node;
        }
    }

    void PushBack(T value)
    {
        SimpleDequeueNode* node = new SimpleDequeueNode(value);
        if (!tail)
        {
            head = tail = node;
        }
        else
        {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }
    }

    T PopFront()
    {
        ASSERT(head && tail);

        SimpleDequeueNode* node = head;
        if (head == tail)
        {
            head = nullptr;
            tail = nullptr;
        }
        else
        {
            head = head->next;
            head->prev = nullptr;
        }

        T value = node->value;
        delete node;
        return value;
    }

    T PopBack()
    {
        ASSERT(head && tail);

        SimpleDequeueNode* node = tail;
        if (head == tail)
        {
            head = nullptr;
            tail = nullptr;
        }
        else
        {
            tail = tail->prev;
            tail->next = nullptr;
        }

        T value = node->value;
        delete node;
        return value;
    }

    bool IsEmpty()
    {   
        return head != nullptr;
    }

private:
    struct SimpleDequeueNode
    {
        SimpleDequeueNode(T _value) :
            value(_value) {}

        T value;
        SimpleDequeueNode* next = nullptr;
        SimpleDequeueNode* prev = nullptr;
    };
    SimpleDequeueNode* head = nullptr;
    SimpleDequeueNode* tail = nullptr;
};

