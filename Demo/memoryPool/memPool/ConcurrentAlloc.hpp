#pragma once
#include "Common.hpp"
#include "ThreadCache.hpp"

void* ConcurrentAlloc(size_t size)
{
	if (size > MAXBYTES)
	{
		//����64Kֱ��malloc
		return malloc(size);
	}
	//��64K֮��ֱ�����̻߳����������ڴ�
	else
	{
		//ͨ��tls����ȡ�߳��Լ���tls
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
			//std::cout << std::this_thread::get_id() << "->"  << tls_threadcache << std::endl;
			//std::cout << tls_threadcache << std::endl;
		}

		//���ػ�ȡ���ڴ��ĵ�ַ
		return tls_threadcache->Allocate(size);
		//return nullptr;
	}
}

//����ȡ�����ڴ��黹����������
void ConcurrentFree(void* ptr, size_t size)
{
	if (size > MAXBYTES)
	{
		return free(ptr);
	}
	else
	{
		return tls_threadcache->Deallocate(ptr, size);
	}
}