#ifndef DEAL_THREAD_H_1022
#define DEAL_THREAD_H_1022
#include "Thread.h"

#include <WinSock2.h>

class DealThread : public Thread
{

public:
	DealThread();
	~DealThread();

	void setClientSocket(SOCKET clntSock);

protected:
	void run();

private:
	void sendData(const char *data, int size);

private:
	SOCKET m_clntSock;
	
};

#endif


