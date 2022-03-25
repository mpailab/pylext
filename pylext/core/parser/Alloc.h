#pragma once
#include <cstdlib>
#include <memory>
template<class T> class AllocRef;

template<class T>
class Alloc{
	union AllocTp{
		char p[sizeof(T)];
		AllocTp *next;
	};
	struct ListAllocNode{
		size_t sz;
		AllocTp* p;
		ListAllocNode *next;
		explicit ListAllocNode(size_t n, ListAllocNode *nxt=0){
			//std::cout << "Allocate " << n << " nodes\n";
			p = (AllocTp*)malloc(n*sizeof(AllocTp));
			for(size_t i=1; i<n; i++)
				p[i-1].next = p+i;
			if(n)p[n-1].next = 0;
			next = nxt;
			sz = n;
		}
		~ListAllocNode(){
			::free(p);
		}
	};

	double exp_coef;
	size_t start_size;
	ListAllocNode *head;
	AllocTp *fr;
	void _check(){
		if(!fr){
			if(head)head = new ListAllocNode(int(head->sz*exp_coef)+1, head);
			else head = new ListAllocNode(start_size);
			fr = head->p;
		}
	}
	void destroy(){
		ListAllocNode *p = head;
		for(;p; p=head){
			head = p->next;
			delete p;
		}
		head = 0;
	}
public:
	Alloc(const Alloc<T>&) = delete;
	Alloc<T>& operator=(const Alloc<T>&a) = delete;
	Alloc(Alloc<T>&&a): exp_coef(a.exp_coef), start_size(a.start_size), head(a.head), fr(a.fr) {
		a.head = 0;
		a.fr = 0;
	}
	Alloc<T>& operator=(Alloc<T>&&a) {
		std::swap(fr, a.fr);
		std::swap(head, a.head);
		std::swap(start_size, a.start_size);
		std::swap(exp_coef, a.exp_coef);
		return *this;
	}
	explicit Alloc(size_t start_sz=10, double k=1.5){
		start_size = start_sz;
		exp_coef = k;
		head = 0;
		fr = 0;
	}
	template<class ... Y>
	T* allocate(Y && ... y){
		_check();
		T *p = (T*)fr;
		fr = fr->next;
		::new(p) T(std::forward(y)...);
		return p;
	}
	void deallocate(T *n){
#ifdef _DEBUG_ALLOCATION
		if(!_check_node(n))throw "Alloc::free : node isn't from current allocator";
#endif
		std::destroy_at(n);
		((AllocTp*)n)->next = fr;
		fr = ((AllocTp*)n);
	}
	~Alloc(){
		destroy();
	}
};
