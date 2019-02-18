#include "PageCache.hpp"

//���ھ�̬�ĳ�Ա����������������г�ʼ��
PageCache PageCache::_inst;

//��PageCache����ȡһ��span������CentralCache
Span* PageCache::NewSpan(size_t npage)
{
	//�������page��span��Ϊ�գ���ֱ�ӷ���һ��span
	if (!_pagelist[npage].Empty())
	{
		return _pagelist[npage].PopFront();
	}

	//�������npage��spanΪ�գ����������������span�ǲ���Ϊ�յģ�������ǿյľͽ����и���span
	for (size_t i = npage + 1; i < NPAGES; i++)
	{
		if (!_pagelist[i].Empty())
		{
			//�����и�
			Span* span = _pagelist[i].PopFront();
			Span* split = new Span();

			//ҳ�ţ���span�ĺ�������и�
			split->_pageid = span->_pageid + span->_npage - npage;
			//ҳ��
			split->_npage = npage;
			span->_npage = span->_npage - npage;

			//���·ָ������ҳ��ӳ�䵽�µ�span��
			for (size_t i = 0; i < npage; i++)
			{
				_id_span_map[split->_pageid + i] = split;
			}

			_pagelist[span->_npage].PushFront(span);

			return split;
		}
	}

	//������Ҳ���ǣ�PageCache����Ҳû�д��������npage��ҳ��Ҫȥϵͳ�����ڴ�
	//���ڴ�ϵͳ�����ڴ棬һ������128ҳ���ڴ棬�����Ļ������Ч�ʣ�һ�����빻����ҪƵ������
	void* ptr = VirtualAlloc(NULL, (NPAGES - 1) << PAGE_SHIFT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	Span* largespan = new Span();
	largespan->_pageid = (PageID)(ptr) >> PAGE_SHIFT;
	largespan->_npage = NPAGES - 1;
	_pagelist[NPAGES - 1].PushFront(largespan);

	//����ϵͳ�����ҳ��ӳ�䵽ͬһ��span
	for (size_t i = 0; i < largespan->_npage; i++)
	{
		_id_span_map[largespan->_pageid + i] = largespan;
	}

	return NewSpan(npage);
}

Span* PageCache::MapObjectToSpan(void* obj)
{
	//ȡ�����ڴ��ҳ��
	PageID pageid = (PageID)(obj) >> PAGE_SHIFT;
	
	auto it = _id_span_map.find(pageid);

	assert(it != _id_span_map.end());

	//���ص�������ڴ��ַҳ��Ϊ�Ǹ���span���ó�����
	return it->second;
}

//��CentralCache��span�黹��PageCache
void PageCache::RelaseToPageCache(Span* span)
{
	//�ҵ����spanǰ���span
	auto previt = _id_span_map.find(span->_pageid - 1);

	while (previt != _id_span_map.end())
	{
		Span* prevspan = previt->second;
		//�ж�ǰ���span�ļ����ǲ���0
		if (prevspan->_usecount != 0)
		{
			break;
		}

		//�ж�ǰ���span���Ϻ����span��û�г���NPAGES
		if (prevspan->_npage + span->_npage >= NPAGES)
		{
			break;
		}

		//���кϲ�
		_pagelist[prevspan->_npage].Erase(prevspan);
		prevspan->_npage += span->_npage;
		delete(span);
		span = prevspan;

		previt = _id_span_map.find(span->_pageid - 1);
	}

	//�ҵ����span�����span
	auto nextvit = _id_span_map.find(span->_pageid + span->_npage);

	while (nextvit != _id_span_map.end())
	{
		Span* nextspan = nextvit->second;
		//�ж�ǰ���span�ļ����ǲ���0
		if (nextspan->_usecount != 0)
		{
			break;
		}

		//�ж�ǰ���span���Ϻ����span��û�г���NPAGES
		if (nextspan->_npage + span->_npage >= NPAGES)
		{
			break;
		}

		//���кϲ�,�������span��span����ɾ�����ϲ���ǰ���span��
		_pagelist[nextspan->_npage].Erase(nextspan);
		span->_npage += nextspan->_npage;
		delete(nextspan);

		nextvit = _id_span_map.find(span->_pageid + span->_npage);
	}

	//���ϲ��õ�ҳ��ӳ�䵽�µ�span��
	for (size_t i = 0; i < span->_npage; i++)
	{
		_id_span_map[span->_pageid + i] = span;
	}

	//��󽫺ϲ��õ�span���뵽span����
	_pagelist[span->_npage].PushFront(span);
}
