// xes_yolo_helper.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>

#include <stdio.h>
#include "ServerManager.h"

using namespace std;

int main()
{
    std::cout << "Hello World!" << std::endl; 

	ServerMgr->instance()->start();

	//while (1);

	return 0;
}
