#include "Common.hpp"
#include "ConcurrentAlloc.hpp"

/*
	tls(thread local storage)�̱߳��ش洢
*/
void TestThreadCache()
{
	//���Բ�ͬ�̶߳���tls��Ч����Ҳ���Ƕ���tlsÿһ���̶߳������Լ�һ��
	//std::thread tl(ConcurrentAlloc, 10);
	//std::thread t2(ConcurrentAlloc, 10);

	//tl.join();
	//t2.join();

	//���������ڴ�
	//ConcurrentAlloc(10);

	size_t n = 2000;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		std::cout << v.back() << std::endl;
	}
	std::cout << std::endl << std::endl;

	/*for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i], 10);
	}
	v.clear();*/

	for (int i = n - 1; i >= 0; --i)
	{
		ConcurrentFree(v[i], 10);
	}
	v.clear();

	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		std::cout << v.back() << std::endl;
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i], 10);
	}
	v.clear();
}


int main()
{
	TestThreadCache();
	return 0;
}