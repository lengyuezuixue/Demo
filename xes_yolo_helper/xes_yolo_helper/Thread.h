#ifndef THREAD_H_1022
#define THREAD_H_1022

#include <process.h>

typedef void *HANDLE;

class Thread
{
public:
	Thread();
	~Thread();

	void start();

	HANDLE getThread();

protected:
	virtual void run();

private:
	HANDLE m_hThread;
	static void agent(void *p);
};


#endif

