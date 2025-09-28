#ifndef CQUEUE_H
#define CQUEUE_H

#define MAXBUFSIZE	1024		// Max Buffer size

#include <stdint.h>
#include <memory.h>

class CQueue
{
public:
    CQueue();
    ~CQueue();

private: // member
    uint16_t m_Head, m_Tail;
    uint8_t m_Buffer[MAXBUFSIZE];

public: // methods
    void Insert(uint8_t recvbuf);
    void Delete(uint16_t size);
    uint8_t GetData(uint16_t index);
    uint16_t GetDataLength();
    uint16_t GetHead();
    uint16_t GetTail();

};

#endif // CQUEUE_H
