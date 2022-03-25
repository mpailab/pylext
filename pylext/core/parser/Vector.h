#pragma once
#include <immintrin.h>
#include <memory>
#include <utility>
#include <vector>
#include "Exception.h"

template <class T, unsigned bsz = 4, class Alloc = std::allocator<T>>
class VectorF {
	alignas(T) char _sdata[bsz * sizeof(T)]{};
	unsigned _size = 0, _cap = bsz;
	T* _ptr = (T*)_sdata;
	Alloc _alloc;
	static_assert((bsz&(bsz - 1)) == 0, "bsz must be power of 2");
public:
	template<class Y>
	class _iterator {
		Y* _ptr;
		friend class VectorF<T,bsz,Alloc>;
		explicit _iterator(Y*p) :_ptr(p){}
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = Y;
		using difference_type = int;
		using pointer = value_type*;
		using reference = value_type&;

		template<class Z, class = std::enable_if_t<std::is_convertible<Z*,Y*>::value>>
		_iterator(const _iterator<Z>&i) :_ptr(i._ptr) {}
		_iterator() :_ptr(0) {}
		pointer operator->() { return _ptr; }
		reference operator*() { return *_ptr; }
		_iterator<Y>& operator++() { ++_ptr; return *this; }
		_iterator<Y>& operator--() { --_ptr; return *this; }
		_iterator<Y>& operator+=(int d) { _ptr += d; return *this; }
		_iterator<Y>& operator-=(int d) { _ptr -= d; return *this; }
		template<class Z>
		bool operator==(const _iterator<Z>&x)const { return x._ptr == _ptr; }
		template<class Z>
		bool operator!=(const _iterator<Z>&x)const { return x._ptr != _ptr; }
	};
	using iterator = _iterator<T>;
	using const_iterator = _iterator<const T>;

	[[nodiscard]] int size() const { return _size; }
	VectorF<T, bsz, Alloc>& reserve(unsigned cap) {
		cap = 1u << (32u - _lzcnt_u32(cap - 1));
		if (cap <= _cap) return *this;
		T* ptr = _alloc.allocate(cap);
		std::uninitialized_move_n(_ptr, _size, ptr);
		std::destroy_n(_ptr, _size);
		if (_size > bsz)_alloc.deallocate(_ptr, _cap);
		_ptr = ptr;
		_cap = cap;
		return *this;
	}
	
	VectorF<T,bsz,Alloc>& push_back(const T&x) {
		if (_size >= _cap)reserve(_size+1);
		new(_ptr + _size) T(x);
		_size++;
		return *this;
	}
	VectorF<T, bsz, Alloc>& push_back(T&&x) {
		if (_size >= _cap)reserve(_size + 1);
		new(_ptr + _size) T(std::move(x));
		_size++;
		return *this;
	}
	T& operator[](size_t i) { return _ptr[i]; }
	const T& operator[](size_t i)const { return _ptr[i]; }
	VectorF<T, bsz, Alloc>& resize(unsigned sz) {
		if (sz < _size) {
			std::destroy_n(_ptr + sz, _size - sz);
		} else if (sz > _size) {
			if (sz > _cap)reserve(sz);
			std::uninitialized_default_construct_n(_ptr + _size, sz-_size);
		}
		_size = (unsigned)sz;
		return *this;
	}
	VectorF<T, bsz, Alloc>& pop_back() {
		_size--;
		std::destroy_at(_ptr + _size);
		return *this;
	}
	const_iterator cbegin()const { return const_iterator(_ptr); }
	const_iterator cend()const { return const_iterator(_ptr + _size); }
	const_iterator begin()const { return const_iterator(_ptr); }
	const_iterator end()const { return const_iterator(_ptr + _size); }
	iterator begin() { return iterator(_ptr); }
	iterator end() { return iterator(_ptr + _size); }

	VectorF() = default;
	VectorF(VectorF<T, bsz, Alloc>&& v) noexcept : _ptr(v._cap <= bsz ? (T*)_sdata : v._ptr),_cap(v._cap),_size(v._size) {
		if (_cap <= bsz) {
			std::uninitialized_move_n(v._ptr, _size, _ptr);
		} else {
			v._cap = bsz;
			v._ptr = (T*)v._sdata;
			v._size = 0;
		}
	}
	VectorF<T, bsz, Alloc>& operator=(VectorF<T, bsz, Alloc>&& v)  noexcept {
		if (v._cap > bsz) {
			if (_cap > bsz) {
				std::swap(_ptr, v._ptr);
				std::swap(_size, v._size);
				std::swap(_cap, v._cap);
			} else {
				std::destroy_n(_ptr, _size);
				_ptr = v._ptr;
				_size = v._size;
				_cap = v._cap;
				v._ptr = (T*)v._sdata;
				v._cap = bsz;
				v._size = 0;
			}
		} else {
			T *q = (T*)_sdata;
			if (_cap > bsz) {
				std::uninitialized_move_n(v._ptr, v._size, q);
				v._ptr = _ptr;
				_ptr = (T*)_sdata;
				v._cap = _cap;
				v._size = _size;
			} else {
				std::destroy_n(_ptr, _size);
				std::uninitialized_move_n(v._ptr, v._size, q);
				_size = v._size;
				_cap = v._cap;
				v._size = 0;
			}
		}
		return *this;
	}
	explicit VectorF(size_t sz) {
		if (sz > bsz) {
			_cap = 1u << (32 - _lzcnt_u32(sz - 1));
			_ptr = _alloc.allocate(_cap);
		}
		std::uninitialized_default_construct_n(_ptr, sz);
		_size = sz;
	}

	template<class It>
	VectorF(It fst, It lst): VectorF(lst-fst) {
		int sz = int(lst - fst);
		if (sz > bsz) {
			_cap = 1u << (32 - _lzcnt_u32(sz - 1));
			_ptr = _alloc.allocate(_cap);
		}
		_size = sz;
		std::uninitialized_copy_n(fst, sz, _ptr);
	}
	~VectorF() {
		std::destroy_n(_ptr, _size);
		if (_cap > bsz)_alloc.deallocate(_ptr, _cap);
		_ptr = 0;
	}
};
