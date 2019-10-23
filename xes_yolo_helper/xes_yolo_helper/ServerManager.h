#ifndef SERVER_MANAGER_H_1021
#define SERVER_MANAGER_H_1021

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>


class ServerManager
{
public:
	static ServerManager *instance();

	void start();
	

private:
	ServerManager();
	~ServerManager();

private:
	int m_port;

	SOCKET m_clntSock;

};

#define ServerMgr  (ServerManager::instance())

#endif




