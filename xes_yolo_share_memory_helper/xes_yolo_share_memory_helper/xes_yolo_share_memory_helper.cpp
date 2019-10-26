// xes_yolo_share_memory_helper.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "SharedMemory.h"

using namespace std;
using namespace sm;


#define BUF_SIZE  4096


int main()
{
    std::cout << "Hello World!\n"; 

	SharedMemory sharedM;
	sharedM.setKey("xes_yolo_share");

	cout << sharedM.key() << "-----"<< sharedM.nativeKey();

	char buffer[BUF_SIZE] = { 0 };
	

	string tt = "hello memory";

	if (sharedM.isAttached()) {
		sharedM.detach();
	}

	if (sharedM.create(40960)) {
		sharedM.lock();

		cout << "FFFFFFFFFF:::" << endl;

		int tt = 5;

		char *to = (char *)sharedM.data();
		memcpy(to, &tt, 4);
		

		sharedM.unlock();

		system("pause");
		sharedM.detach();
	}

	SystemSemaphore ssm("xes_yolo_ssm", 1, SystemSemaphore::Open);
	if (ssm.acquire()) {
		cout << "TTTTTTTTTTTTTTTTTT" << endl;
	}


	system("pause");
}


