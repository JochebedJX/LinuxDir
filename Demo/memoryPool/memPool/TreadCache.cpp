#include "ThreadCache.hpp"
#include "CentralCache.hpp"

//�����Ļ�����ȡ����
void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
	//��ȡ���ڴ�����������������
	FreeList* freelist = &_freelist[index];

	//ÿ�δ����Ļ���ȡ��10��������С���ڴ��
	size_t num = 10;

	//start��end�ֱ��ʾȡ�������ڴ�Ŀ�ʼ��ַ�ͽ�����ַ
	//ȡ�������ڴ���һ���������ڴ�
	void* start, *end;

	//fetchnum��ʾʵ��ȡ�������ڴ�ĸ�����fetchnum�п���С��num����ʾ���Ļ���û����ô���С���ڴ��
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, byte);
	if (fetchnum == 1)
	{
		return start;
	}

	//ֻ�����������з���(fetchnum - 1)���ڴ�飬������Ϊ��һ���ڴ���Ѿ���ʹ����
	freelist->PushRange(NEXT_OBJ(start), end, fetchnum - 1);
	return start;
}

//���������������������������ȡ�ڴ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size < MAXBYTES);
	//size��Ҫ��ȡ���ٸ��ֽڵ��ڴ�
	size = ClassSize::RoundUp(size);
	//index�����������������±�
	size_t index = ClassSize::Index(size);

	//����ȷ��Ҫ��ȡ��size�Ǵ������������������һ����������
	FreeList* freelist = &_freelist[index];

	//�ж������Ƿ�Ϊ��
	//��Ϊ�գ�ֱ��ȡ�ڴ�
	if (!freelist->Empty())
	{
		//Ҳ���Ǵ��������������ڴ�
		return freelist->Pop();
	}
	//���������ǿյ�Ҫ�����Ķ�����ȡ�ڴ棬һ��ȡ�����ֹ�ظ�ȡ
	else 
	{
		return FetchFromCentralCache(index, size);
	}
}

//����������������е���������黹�ڴ�
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size < MAXBYTES);
	size_t index = ClassSize::Index(size);

	FreeList* freelist = &_freelist[index];

	freelist->Push(ptr);
}
