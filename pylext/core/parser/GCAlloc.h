#pragma once
#include <cstdlib>
#include <memory>
#include <iterator>
#include <type_traits>


template<typename N>
class GCPtr {
	N* _p = 0;
public:
	GCPtr() = default;
	explicit GCPtr(N* p) {
		if((_p = p))
			p->refs++;
	}
	GCPtr(const GCPtr& x) {
		if((_p = x._p))
			_p->refs++;
	}
	GCPtr(GCPtr&& x) noexcept : _p(x._p) {
		x._p = 0;
	}
	GCPtr& operator=(const GCPtr& x) {
		if (_p)_p->refs--;
		if ((_p = x._p))_p->refs++;
		return *this;
	}
	GCPtr& operator=(GCPtr&& x)  noexcept {
		std::swap(_p, x._p);
		return *this;
	}
	void reset(N* p = 0){
		if (_p)_p->refs--;
		if ((_p = p))p->refs++;
	}
	N* get()const { return _p; }
	N* operator->()const { return _p; }
	N& operator*()const { return *_p; }
	N& operator[](size_t i) { return *_p->ch[i]; }
	~GCPtr() { if (_p)_p->refs--; }
};

template<typename T>
class GCAlloc {
	struct GCAllocTp {
		union {
			char p[sizeof(T)];
			GCAllocTp* next;
		};
		bool free = true;
	};
	vector<T*> _temp_gc;
	struct ListAllocNode {
		size_t sz;
		GCAllocTp* p;
		ListAllocNode* next;
		explicit ListAllocNode(size_t n, ListAllocNode* nxt = 0) {
			p = (GCAllocTp*)malloc(n * sizeof(GCAllocTp));
			
			for (size_t i = 1; i < n; i++) {
				p[i - 1].next = p + i;
				p[i].free = true;
			}
			if (n) {
				p[n - 1].next = 0;
				p[0].free = true;
			}
			next = nxt;
			sz = n;
		}
		size_t mark_unused() {
			size_t n = 0;
			for (size_t i = 0; i < sz; i++)
				if (!p[i].free) {
					((T*)(p + i))->used = 0;
					n++;
				}
			if (next)n+=next->mark_unused();
			return n;
		}
		void find_top(vector<T*>& tmp, size_t &pos) {
			for (size_t i = 0; i < sz; i++)
				if (!p[i].free && ((T*)(p + i))->refs > 0) {
					T* pi = (T*)(p + i);
					pi->used = 1;
					tmp[pos++] = pi;
				}
			if (next)next->find_top(tmp, pos);
		}
		void del_unused(GCAlloc& alloc) {
			for (size_t i = 0; i < sz; i++)
				if (!p[i].free && !((T*)(p + i))->used)
					alloc.deallocate((T*)(p + i));
			if (next)next->del_unused(alloc);
		}
		void rungc(GCAlloc& alloc, vector<T*> &tmp) {
			size_t n = mark_unused();
			tmp.resize(n);
			size_t c = 0;
			find_top(tmp, c);
			for (size_t i = 0; i < c; i++) {
				for(T *x: tmp[i]->ch)
					if (!x->used) {
						x->used = 1;
						tmp[c++] = x;
					}
			}
			del_unused(alloc);
		}
		~ListAllocNode() {
			for (size_t i = 0; i < sz; i++)
				if (!p[i].free) {
					destroy_at((T*)(p + i));
				}
			::free(p);
		}
	};

	double exp_coef;
	size_t start_size;
	size_t _allocated = 0;
	size_t _aftergc = 0;
	ListAllocNode* head;
	GCAllocTp* fr;
	void _check() {
		if (!fr) {
			if (_aftergc*2 > _allocated)_rungc();
			if (fr)return;
			if (head)head = new ListAllocNode(int(head->sz * exp_coef) + 1, head);
			else head = new ListAllocNode(start_size);
			_allocated += head->sz;
			fr = head->p;
		}
	}
	void _rungc() {
		head->rungc(*this, _temp_gc);
		_aftergc = 0;
	}
	void destroy() {
		ListAllocNode* p = head;
		for (; p; p = head) {
			head = p->next;
			delete p;
		}
		head = 0;
	}
public:
	GCAlloc(const GCAlloc<T>&) = delete;
	GCAlloc<T>& operator=(const GCAlloc<T>& a) = delete;
	GCAlloc(GCAlloc<T>&& a)  noexcept : exp_coef(a.exp_coef), start_size(a.start_size), head(a.head), fr(a.fr) {
		a.head = 0;
		a.fr = 0;
	}
	GCAlloc<T>& operator=(GCAlloc<T>&& a)  noexcept {
		std::swap(fr, a.fr);
		std::swap(head, a.head);
		std::swap(start_size, a.start_size);
		std::swap(exp_coef, a.exp_coef);
		return *this;
	}
	explicit GCAlloc(size_t start_sz = 10, double k = 1.5) {
		start_size = start_sz;
		exp_coef = k;
		head = 0;
		fr = 0;
	}
	template<class ... Y>
	T* allocate(Y && ... y) {
		_check();
		T* p = (T*)fr;
		fr->free = false;
		fr = fr->next;
		::new(p) T(std::forward(y)...);
		_aftergc++;
		return p;
	}
	void deallocate(T* n) {
#ifdef _DEBUG_ALLOCATION
		if (!_check_node(n))throw "Alloc::free : node isn't from current allocator";
#endif
		std::destroy_at(n);
		((GCAllocTp*)n)->next = fr;
		((GCAllocTp*)n)->free = true;
		fr = ((GCAllocTp*)n);
	}
	~GCAlloc() {
		destroy();
	}
};
