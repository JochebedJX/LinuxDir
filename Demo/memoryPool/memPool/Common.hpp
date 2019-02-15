#pragma once
#include <iostream>
#include <assert.h>
#include <thread>
#include <mutex>
#include <vector>


//����������������ĳ���
const size_t NLISTS = 240;
//������һ�η�������ڴ�64K
const size_t MAXBYTES = 64 * 1024;


static inline void*& NEXT_OBJ(void* obj)
{
	return *((void**)obj);
}

//��������
class FreeList
{
public:
	bool Empty()
	{
		return _list == nullptr;
	}

	void PushRange(void* start, void* end, size_t num)
	{
		//_list->start->end->_list
		//����һ���ڴ���ӵ����������У����Ҷ��ڸö�����������ڴ��������������
		NEXT_OBJ(end) = _list;
		_list = start;
		_size += num;
	}

	void* Pop()
	{
		void* obj = _list;
		_list = NEXT_OBJ(obj);
		_size--;

		return obj;
	}

	void Push(void* ptr)
	{
		NEXT_OBJ(ptr) = _list;
		_list = ptr;
		_size++;
	}

private:
	void* _list = nullptr;//�γ�һ����������
	size_t _size = 0;//�ж��ٸ��ڴ���
	size_t _maxsize = 0;//����ж��ٸ��ڴ���
};

//����span��Ϊ�˶���thread cache���������ڴ���й���
//һ��span�а������ڴ��
typedef size_t PageID;
struct Span
{
	PageID _id; //ҳ��
	size_t _npage; //ҳ������
	Span* _next;
	Span* _prev;

	void* _objlist = nullptr; //������������
	size_t _objsize = 0;	//��¼��span�ϵ��ڴ��Ĵ�С
	size_t _usecount = 0; //ʹ�ü���������ʹ���˶����ڴ��
};

//�������
class SpanList
{
public:
	//˫��ѭ����ͷ�������
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* begin()
	{
		return _head->_next;
	}
	
	Span* end()
	{
		return _head;
	}

	bool Empty()
	{
		return _head == _head->_next;
	}

	void Insert(Span* cur, Span* newspan)
	{
		assert(cur);
		Span* prev = cur->_prev;
		
		//prev newspan cur
		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Erase(Span* cur)
	{
		assert(cur != nullptr && cur != _head);

		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	void PushFront(Span* cur)
	{
		Insert(begin(), cur);
	}

	Span* PopFront()
	{
		//����Ҫʹ��sapn������һ�¿�ʼ����Ϊɾ��֮���ٴ�ʹ��begin()���صĻ����ͻ᷵�ظղ�ɾ������һ��
		//���������ɾ�����Ǹ�һ����
		Span* span = begin();
		Erase(span);

		return span;
	}

private:
	Span * _head = nullptr;
};

class ClassSize
{
public:
	//align�Ƕ�����
	static inline size_t _RoundUp(size_t size, size_t align)
	{
		//����size��15 < 128,������align��8����ôҪ��������ȡ����
		//((15 + 7) / 8) * 8�Ϳ�����
		//���ʽ�Ӿ��ǽ�(align - 1)����ȥ�������Ļ��Ϳ��Խ�һ����������
		//Ȼ���ٽ�����ȥ�Ķ����Ƶĵ���λ����Ϊ0��Ҳ��������ȡ����
		//15 + 7 = 22 : 10110 (16 + 4 + 2)
		//7 : 111 ~7 : 000
		//22 & ~7 : 10000 (16)�ʹﵽ������ȡ����Ч��
		return (size + align - 1) & ~(align - 1);
	}

	//����ȡ��
	static inline size_t RoundUp(size_t size)
	{
		assert(size <= MAXBYTES);
		
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		if (size <= 8 * 128)
		{
			return _RoundUp(size, 16);
		}
		if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		if (size <= 64 * 1024)
		{
			return _RoundUp(size, 512);
		}
		else
		{
			return -1;
		}
	}

	//��������Ƭ��12%���ҵ��˷�
	//[1, 128]						8byte����		freelist[0,16)
	//[129, 1024]					16byte����		freelist[17, 72)
	//[1025, 8 * 1024]				64byte����		freelist[72, 128)
	//[8 * 1024 + 1, 64 * 1024]		512byte����		freelist[128, 240)
	//Ҳ����˵����������������ֻ��Ҫ����240���ռ�Ϳ�����

	//����ڸ�����ĵڼ���
	static size_t _Index(size_t bytes, size_t align_shift)
	{
		//����(1 << align_sjift)�൱�����������
		//��bytes���϶�������һҲ���ǣ�������Կ�Խ����һ����������������Ԫ����
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	//��ȡ����������±�
	static inline size_t Index(size_t bytes)
	{
		//���ٵ��ֽ���������С�ڿ��Կ��ٵ������ֽ���
		assert(bytes < MAXBYTES);

		//ÿ�����������У����Ŷ�������������
		static int group_array[4] = { 16, 56, 56, 112 };

		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) //(8 * 128)
		{
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 4096) //(8 * 8 * 128)
		{
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 8 * 128)
		{
			return _Index(bytes - 4096, 9) + group_array[2] + group_array[1] + group_array[0];
		}
		else
		{
			return -1;
		}
	}

};