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
	WSADATA wsaData;							//����һ���ṹ�����
	WSAStartup(MAKEWORD(2, 2), &wsaData);		//��ʼ��WSAStartup()����,,,(�淶�İ汾�ţ�ָ��WSADATA�ṹ���ָ��),,,MSKEWORD(2,2)���汾��2�����汾��2

	//...������1��IP��ַ����PF_INET6ΪIPv6����2�����ݴ��䷽ʽ SOCK_STREAM �� SOCK_DGRAM ��3������Э�� IPPROTO_TCP �� IPPTOTO_UDP�����д0ϵͳ���Զ����㴦ʹ������Э��
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
