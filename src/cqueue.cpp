#include "cqueue.h"

CQueue::CQueue()
{
    m_Head = 0;
    m_Tail = 0;
    memset(m_Buffer, 0, sizeof(m_Buffer));
}

CQueue::~CQueue()
{

}

void CQueue::Insert(uint8_t recvbuf)
{
    m_Buffer[m_Head++] = recvbuf;
    m_Head &= (MAXBUFSIZE - 1);
    if(m_Head == m_Tail)
    {
        m_Tail++;
        m_Tail &= (MAXBUFSIZE - 1);
    }
}

void CQueue::Delete(uint16_t size)
{
    m_Tail += size;
    m_Tail &= (MAXBUFSIZE - 1);
}

uint8_t CQueue::GetData(uint16_t index)
{
    uint16_t lindex = index;
    lindex &= (MAXBUFSIZE - 1);
    return m_Buffer[lindex];
}

uint16_t CQueue::GetDataLength()
{
    uint16_t ret = 0;
    if(m_Head >= m_Tail) ret = m_Head - m_Tail;
    else ret = MAXBUFSIZE - m_Tail + m_Head;
    return ret;
}

uint16_t CQueue::GetHead()
{
    return m_Head;
}

uint16_t CQueue::GetTail()
{
    return m_Tail;
}
