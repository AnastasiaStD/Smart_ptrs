#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

template <typename T>
class EnableSharedFromThis;

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), block_(nullptr){};

    SharedPtr(std::nullptr_t) : ptr_(nullptr), block_(nullptr){};

    SharedPtr(T* ptr, ControlBlockBase* block) : ptr_(ptr), block_(block){};

    explicit SharedPtr(T* ptr) : ptr_(ptr) {
        ControlBlockBase* a = new ControlBlockPointerImpl<T>(ptr);
        block_ = a;
    };

    template <typename U>
    explicit SharedPtr<T>(U* ptr) : ptr_(ptr) {
        block_ = new ControlBlockPointerImpl<U>(ptr);;
    };

    SharedPtr(const SharedPtr<T>& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementStrong();
        }
    };

    template <typename U>
    SharedPtr(SharedPtr<U>& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementStrong();
        }
    };

    template <typename U>
    SharedPtr(SharedPtr<U>&& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) noexcept : ptr_(ptr), block_(other.block_) {
        if (block_) {
            block_->IncrementStrong();
        }
    };

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_ && !block_->ExistsStrong()) {
            throw BadWeakPtr();
        }
        //        if (block_) {
        //           block_->IncrementStrong();
        //        }
        *this = std::move(other.Lock());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncrementStrong();
            }
        }
        return *this;
    };


    template <typename U>
    SharedPtr<U>& operator=(const SharedPtr<T>& other) {
        if (this->ptr_ != other.ptr_) {
            Reset();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncrementStrong();
            }
        }
        return *this;
    };

    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            this->Reset();
            ptr_ = other.ptr_;
            block_ = other.block_;
            // other.Reset();
            other.ptr_ = nullptr;
            other.block_ = nullptr;
        }
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers
    void Reset() {
        if (block_) {
            if (block_->GetStrong() == 1) {
                block_->ZeroStrong();
                ptr_ = nullptr;
                if (!block_->ExistsWeak()) {
                    block_->ZeroWeak();
                    block_ = nullptr;
                    return;
                }
            }
            block_->DecrementStrong();
        }
    };

    void Reset(T* ptr) {
        if (ptr_ != ptr) {
            Reset();
            auto nnew = new ControlBlockPointerImpl<T>(ptr);
            block_ = nnew;
            if (ptr_) {
                block_->Put();
            }
            ptr_ = ptr;
        }
    };

    template <typename U>
    void Reset(U* ptr) {
        if (ptr_ != ptr) {
            Reset();

            if (ptr != nullptr) {
                auto nnew = new ControlBlockPointerImpl<U>(ptr);
                block_ = nnew;
                if (ptr_) {
                    block_->Put();
                }
                ptr_ = static_cast<U*>(ptr);
            }
        }
    };



    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
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
        if (block_) {
            return block_->GetStrong();
        } else {
            return 0;
        }
    };
    explicit operator bool() const {
        return ptr_ != nullptr;
    };

private:
    T* ptr_;
    ControlBlockBase* block_;

    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    friend class ControlBlockBase;

    template <typename Y>
    void SharedFromThisHelper(EnableSharedFromThis<Y>* e) {
        e->self_ = *this;
    }
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
};

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto block = new ControlBlockEmplaceImpl<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(block->GetRawPtr(), block);
};

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(this->weak_this_);
    };
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(this->weak_this_);
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(this->weak_this_);
    };
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(this->weak_this_);
    };

protected:
    EnableSharedFromThis() = default;

    EnableSharedFromThis(const EnableSharedFromThis&) {
    }

    EnableSharedFromThis& operator=(const EnableSharedFromThis&) {
        return *this;
    }

    ~EnableSharedFromThis() {
    }

private:
    mutable WeakPtr<T> weak_this_;
};
