#pragma once

class LinkedList
{
public:
    struct _LL_NODE
    {
        template <typename T>
        T *GetDataAs()
        {
            return (T *)m_pvData;
        }

        void *m_pvData;
        _LL_NODE *m_Next;
        _LL_NODE *m_Prev;
    };

    int m_NodeC;
    _LL_NODE *m_Head;
    _LL_NODE *m_Tail;

    LinkedList();
    void AddToHead(void *pvData);
    void AddToTail(void *pvData);
    void RemoveNode(_LL_NODE *pNodeToRemove);
    _LL_NODE *RemoveHeadNode();
    void ClearList();
};