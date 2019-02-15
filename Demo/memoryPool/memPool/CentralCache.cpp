#include "CentralCache.hpp"

//�Ծ�̬��Ա�������г�ʼ��
CentralCache CentralCache::_inst;

//��׮��дһ�β���һ�Σ�
//�����Ļ����ȡһ�������Ķ����thread cache�����в���ֱ��ʹ��malloc���з���
//size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num ,size_t bytes)
//{
//	//����num���ڴ��СΪbyte���ڴ��
//	start = malloc(bytes * num);
//	end = (char*)start + bytes * (num - 1);
//	void* cur = start;
//
//	//�����з����num���ڴ����������
//	while (cur <= end)
//	{
//		void* next = (char*)cur + bytes;
//		NEXT_OBJ(cur) = next;
//
//		cur = next;
//	}
//
//	NEXT_OBJ(end) = nullptr;
//
//	return num;
//}

Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	//���ڸ�span��������span��Ϊ�յ�spanֱ�ӷ���
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span->_objlist != nullptr)
		{
			return span;
		}
		span = span->_next;
	}

	//�����е�span�����е�����span�����궼Ϊ�գ���ҳ����������span
}


//�����Ļ����ȡ�ڴ���thread cache�����Ļ����Ǵ�ҳ�����������ڴ棨span��
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t bytes)
{
	//�����span���������е��±�
	size_t index = ClassSize::Index(bytes);

	//�ó���Ӧbytesһ��span����,���Ӹ������У��ó�һ��span������thread cache�����ڴ����
	SpanList* spanlist = &_spanlist[index];
	Span* span = GetOneSpan(spanlist, bytes);
	
	void* cur = span->_objlist;
	//prev��Ϊ�˻�û�ȡ��������ڴ�����һ���ڴ��
	void* prev= cur;
	size_t fetchnum = 0;
	while (cur != nullptr && fetchnum <= num)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		fetchnum++;
	}

	start = span->_objlist;
	//����end���ڴ�����һ�飬���Ƕ������һ��ĵ�ַ���������һ���ڴ���ǰ4������8���ֽ�
	end = prev;

	//Ҫ��ȡ��������һ���������ڴ������Ҫ�ڼ�¼�ڴ��
	//������Ϊ����span�����п����ж���ڴ�飬������û�����꣬���Ծͽ��и�nullptr
	NEXT_OBJ(end) = nullptr;

	//��ʣ�������ڴ���ٴ�����span��
	span->_objlist = NEXT_OBJ(prev);

	span->_usecount += fetchnum;

	return fetchnum;
}