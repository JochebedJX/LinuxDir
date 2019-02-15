/*
	Ϊ�˽�������̻߳�ȡ�ڴ�ʱ�������ľ���������˲��������µ�����
*/

#pragma once
#include "Common.hpp"
#include "CentralCache.hpp"
#include <iostream>
#include <stdlib.h>

class ThreadCache
{
public:
	//���̷߳����ڴ�
	void* Allocate(size_t size);

	//�ͷ��ڴ�
	void Deallocate(void* ptr, size_t size);

	//�����Ļ�������ȡ�ڴ棬index�������±꣬size��Ҫ��ȡ�ڴ���ֽڴ�С
	void* FetchFromCentralCache(size_t index, size_t size);

private:
	FreeList _freelist[NLISTS];//������һ�������������飬����ΪNLISTS��240
	/*int _tid;
	ThreadCache* _next;*/
};

//��̬��tls����������ÿһ��ThreadCache���������Լ���һ��tls_threadcache
//�����˶���ÿһ���̶߳����Լ���threadcache
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;