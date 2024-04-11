#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    };
    size_t DecRef() {
        if (count_ == 0) {
            return count_;
        }
        --count_;
        return count_;
    };
    size_t RefCount() const {
        return count_;
    };

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        static_cast<Derived*>(this)->counter_.IncRef();
        // counter_.IncRef();
    };

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (static_cast<Derived*>(this)->counter_.DecRef() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    };

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return static_cast<const Derived*>(this)->counter_.RefCount();
        // counter_.RefCount();
    };

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    T* ptr_;
    template <typename Y>
    friend class IntrusivePtr;

    template <typename Y, typename... Args>
    friend IntrusivePtr<Y> MakeIntrusive(Args&&... args);

public:
    // Constructors
    IntrusivePtr() : ptr_(nullptr){};
    IntrusivePtr(std::nullptr_t) : ptr_(nullptr){};
    IntrusivePtr(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            ptr_->IncRef();
        }
    };

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->IncRef();
        }
    };

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    };

    IntrusivePtr(const IntrusivePtr& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->IncRef();
        }
    };
    IntrusivePtr(IntrusivePtr&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    };

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (ptr_ != other.ptr_) {
            IntrusivePtr{other}.Swap(*this);
        }
        return *this;
    };
    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (ptr_ != other.ptr_) {
            Reset(other.Release());
        }
        return *this;
    };

    // Destructor
    ~IntrusivePtr() {
        if (ptr_) {
            ptr_->DecRef();
        }
    };

    // Modifiers
    void Reset() {
        // IntrusivePtr{}.Swap(*this);
        if (ptr_) {
            ptr_->DecRef();
            ptr_ = nullptr;
        }
    };
    void Reset(T* ptr) {
        // IntrusivePtr{ptr}.Swap(*this);
        if (ptr_ != ptr) {
            Reset();
            ptr_ = ptr;
        }
    };

    T* Release() {
        T* result = ptr_;
        ptr_ = nullptr;
        return result;
    }
    void Swap(IntrusivePtr& other) {
        std::swap(ptr_, other.ptr_);
    };

    // Observers
    T* Get() const {
        return ptr_;
    };
    T& operator*() const {
        return *ptr_;
    };
    T* operator->() const {
        return ptr_;
    };
    size_t UseCount() const {
        return ptr_ ? ptr_->RefCount() : 0;
    };
    explicit operator bool() const {
        return ptr_ != nullptr;
    };
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    auto a = new T(std::forward<Args>(args)...);
    return IntrusivePtr<T>(a);
}
