#include "pch.h"
#include "Thread.h"

#include <iostream>

Thread::Thread()
{

}

Thread::~Thread()
{

}

void Thread::start()
{
	m_hThread = (HANDLE)_beginthread(agent, 0, (void *)this);
}

void Thread::run()
{
	std::cout << "Base Thread" <<std::endl;
}

void Thread::agent(void *p)
{
	Thread *agt = (Thread *)p;
	if (agt) {
		agt->run();
	}
}


HANDLE Thread::getThread()
{
	return m_hThread;
}
