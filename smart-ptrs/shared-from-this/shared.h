#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "weak.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <typename T>
class EnableSharedFromThis;

class ESFTBase {};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), block_(nullptr){};

    SharedPtr(std::nullptr_t) : ptr_(nullptr), block_(nullptr){};

    SharedPtr(T* ptr, ControlBlockBase* block) : ptr_(ptr), block_(block) {
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            ptr->weak_this_ = WeakPtr(*this);
        }
    };

    explicit SharedPtr(T* ptr) : ptr_(ptr) {
        ControlBlockBase* a = new ControlBlockPointerImpl<T>(ptr);
        block_ = a;
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            ptr->weak_this_ = WeakPtr(*this);
        }
    };

    template <typename U>
    explicit SharedPtr<T>(U* ptr) : ptr_(ptr) {
        block_ = new ControlBlockPointerImpl<U>(ptr);
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            ptr->weak_this_ = WeakPtr(*this);
        }
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
        if (block_) {
            block_->IncrementStrong();
        }
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
        if (block_ && ptr_) {
            if (block_->GetStrong() == 1) {
                block_->DecrementStrong();
                ptr_ = nullptr;
                block_ = nullptr;
            } else {
                block_->DecrementStrong();
            }
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

template <typename T>
class EnableSharedFromThis : public ESFTBase {
public:
    SharedPtr<T> SharedFromThis() {
        auto e = weak_this_.Lock();
        return SharedPtr<T>(e);
    };
    SharedPtr<const T> SharedFromThis() const {
        auto e = weak_this_.Lock();
        if (e) {
            return SharedPtr<const T>(e);
        } else {
            throw BadWeakPtr();
        }
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    };
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    };

    mutable WeakPtr<T> weak_this_;

protected:
    EnableSharedFromThis() = default;

    EnableSharedFromThis(const EnableSharedFromThis&) = default;

    EnableSharedFromThis& operator=(const EnableSharedFromThis&) = default;

    ~EnableSharedFromThis() = default;
};