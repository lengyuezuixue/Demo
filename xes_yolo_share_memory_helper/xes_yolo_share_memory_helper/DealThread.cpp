#include "pch.h"
#include "DealThread.h"


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <windows.h>

/*1280*720*3 = 2764800*/
/*5*4 = 20*/
/*2764800  + 20 = 2764820 + 80 = 2764900*/

#define PACKAGE_MAX_SIZE  404300

#define PACKAGE_HEAD_SIZE   20
#define RECV_DATA_MAX_SIZE  404300

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


	static int nRemainSize = 0;
	static char szLastSaveData[PACKAGE_MAX_SIZE];
	memset(szLastSaveData, 0, sizeof(szLastSaveData));

	static char recvData[RECV_DATA_MAX_SIZE];

	int packageSize = 0;
	static char pixSaveData[PACKAGE_MAX_SIZE];

	int flag = 0, width = 0, height = 0, rgb = 0, size = 0;
	while (true) {
		memset(recvData, 0, RECV_DATA_MAX_SIZE);
		int nRecSize = recv(m_clntSock, recvData, RECV_DATA_MAX_SIZE, 0);
		cout << "recv size :" << nRecSize << endl;

		if (nRecSize > 0) {
			memcpy(szLastSaveData + nRemainSize, recvData, nRecSize);
			nRemainSize += nRecSize;

			while (nRemainSize) {
				if (nRemainSize < PACKAGE_HEAD_SIZE) {
					break;
				}

				memcpy(&size, &szLastSaveData[16], 4);
				packageSize = PACKAGE_HEAD_SIZE + size;

				cout << "package size::" << packageSize << endl;;

				//如果不够长度等够了在来解析
				if (nRemainSize < packageSize || size <= 0) {
					break;
				}

				//memcpy(pixSaveData, szLastSaveData + 20, size);

				memcpy(pixSaveData, szLastSaveData, packageSize);

				memcpy(&flag, &szLastSaveData[0], 4);

				if (flag == 0) {
					sendData(pixSaveData, packageSize);
				}


				for (int i = packageSize; i < nRemainSize; ++i) {
					szLastSaveData[i - packageSize] = szLastSaveData[i];
				}

				nRemainSize -= packageSize;

			}
		}
		else {
			break;
		}
	}


}

void DealThread::sendData(const char *data, int size)
{
	if (m_clntSock > 0) {
		cout << "server send data size : " << size << endl;
		send(m_clntSock, data, size, NULL);
	}
}