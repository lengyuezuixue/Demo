#include "pch.h"
#include "DealThread.h"


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <windows.h>

/*1280*720*3 = 2764800*/
/*5*4 = 20*/
/*2764800  + 20 = 2764820 + 80 = 2764900*/

#define MAX_SIZE 404300

using namespace std;

DealThread::DealThread()
	: Thread()
	, m_clntSock(0)
{

}

DealThread::~DealThread()
{

}

uint32_t swap_endian(uint32_t val) {
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

void DealThread::setClientSocket(SOCKET clntSock)
{
	m_clntSock = clntSock;
}

void DealThread::run()
{
	if (m_clntSock == 0) {
		return;
	}

	char recvData[MAX_SIZE];
	char data[MAX_SIZE];

	int recvSize = 0, dataSize = 0;

	int flag = 0, width = 0, height = 0, rgb = 0, size = 0;

	while (true) {
		memset(recvData, 0, MAX_SIZE);
		int ret = recv(m_clntSock, recvData, MAX_SIZE, 0);
		cout << "recv size :" << ret << endl;
		
		if (ret > 0)
		{
			if (recvSize == 0) {
				memcpy(&size, &recvData[16], 4);
				size = swap_endian(size);

				dataSize = size + 20;

				cout <<" size:" << size << endl;

				
			}

			if (recvSize != dataSize) {
				memcpy(&data[recvSize], &recvData[0], ret);
				recvSize += ret;
			}

			if (recvSize == dataSize && recvSize != 0) {
				recvSize = 0;
				dataSize = 0;

				memcpy(&flag, &data[0], 4);
				memcpy(&width, &data[4], 4);
				memcpy(&height, &data[8], 4);
				memcpy(&rgb, &data[12], 4);
			

				flag = swap_endian(flag);
				width = swap_endian(width);
				height = swap_endian(height);
				rgb = swap_endian(rgb);

				cout << "flag:" << flag << " width:" << width << " height:" << height << "rgb:" << rgb  << endl;

			}


			sendData(recvData, ret);
		}
		else {
			break;
		}
	}
}

void DealThread::sendData(const char *data, int size)
{
	if (m_clntSock > 0) {
		send(m_clntSock, data, size, NULL);
	}
}