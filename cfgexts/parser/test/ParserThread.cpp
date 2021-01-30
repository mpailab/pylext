//
// Created by Gleb on 15.01.2021.
//
#include <iostream>
#if 1
#include <coroutine>
#else
#include <experimental/coroutine>
using namespace std::experimental;
#endif
using namespace std;
//#include <coroutine>

template<typename T>
class Generator{
public:
    struct Promise {
        T current;
        auto get_return_object() {return Generator<T>(this); }
        auto initial_suspend() const noexcept{ return suspend_always{}; }
        auto final_suspend() noexcept { return suspend_never{}; }
        auto yield_value(T val) noexcept{
            current = std::move(val);
            return suspend_always{};
        }
        void unhandled_exception() {
            throw std::exception();
        }
        void return_void(){}
        template<class U>
        suspend_never await_transform(U&& v) = delete;
    };
    using promise_type = Promise;
    explicit Generator(promise_type *p):_p(p), handle(coroutine_handle<Promise>::from_promise(*p)){}
    class iterator {
        Promise *p = 0;
        coroutine_handle<> *handle;
    public:
        iterator() = default;
        explicit iterator(promise_type* _p, coroutine_handle<> *h): p(_p), handle(h) {}
        iterator& operator++() {
            handle->resume();
            if(handle->done()) //handle->done())
                p = 0;
            return *this;
        }
        bool operator != (const iterator& it) const {
            return p != it.p;
        }
        bool operator != (const nullptr_t) const {
            return p != 0;
        }
        const T& operator*()const{
            return p->current;
        }
    };
    [[nodiscard]] iterator begin() {
        handle.resume();
        return iterator(_p, &handle);
    }
    std::nullptr_t end() const {return nullptr;}
private:
    coroutine_handle<> handle;
    promise_type* _p;
};

Generator<int> rng(int n){
    for (int i=0; i<n; i++) {
        // cout<<"before yield "<<i<<endl;
        // co_await std::suspend_never{};
        co_yield i;
        // cout<<"after yield "<<i<<endl;
    }
    // cout<<"before return"<<endl;
}

int print_rng(){
    for(auto x : rng(10))
        cout<<x<<endl;
    exit(0);
    return 0;
}

//static int call = print_rng();