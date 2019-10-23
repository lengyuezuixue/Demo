#include "pch.h"
#include "ServerManager.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

#include <iostream>

#include "DealThread.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

ServerManager::ServerManager()
{
	m_port = 55920;
	m_clntSock = SOCKET_ERROR;
}


ServerManager::~ServerManager()
{
	if (m_clntSock != SOCKET_ERROR) {
		closesocket(m_clntSock);
		WSACleanup();
	}
}

ServerManager *ServerManager::instance()
{
	static ServerManager _instance;
	return &_instance;
}

void ServerManager::start()
{
	WSADATA wsaData;							//定义一个结构体对象
	WSAStartup(MAKEWORD(2, 2), &wsaData);		//初始化WSAStartup()函数,,,(规范的版本号，指向WSADATA结构体的指针),,,MSKEWORD(2,2)主版本号2，副版本号2

	//...参数（1）IP地址类型PF_INET6为IPv6，（2）数据传输方式 SOCK_STREAM 和 SOCK_DGRAM （3）传输协议 IPPROTO_TCP 和 IPPTOTO_UDP，如果写0系统会自动计算处使用那种协议
	SOCKET servSock = socket(PF_INET, SOCK_STREAM, 0);
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_port = htons(m_port);
	inet_pton(AF_INET, "127.0.0.1", (void*)&sockAddr.sin_addr.S_un.S_addr);


	while (1) {
		sockAddr.sin_port = htons(m_port);
		bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
		cout << "bind socket success" << endl;

		if (0 == listen(servSock, 1)) {
			std::cout << "listen port:" << m_port << std::endl;
			break;
		}

		++m_port;
	}
	
	cout << "server is Listening......." << endl;

	SOCKADDR clntAddr;
	int nSize = sizeof(SOCKADDR);
	m_clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &nSize);

	cout << "Client connection succeeded...." << endl;

	DealThread *delThread = new DealThread();
	delThread->setClientSocket(m_clntSock);
	delThread->start();

	WaitForSingleObject(delThread->getThread(), INFINITE);

}
