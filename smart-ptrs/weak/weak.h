#pragma once

#include "sw_fwd.h"  // Forward declaration

template <typename T>
class WeakPtr {
public:
    WeakPtr() : ptr_(nullptr), block_(nullptr){};

    WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementWeak();
        }
    }
    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementWeak();
        }
    }

    template <typename Y>
    WeakPtr(WeakPtr<Y>&& other) : ptr_(other.ptr_), block_(std::move(other.block_)) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    template <typename Y>
    WeakPtr(const SharedPtr<Y>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementWeak();
        }
    }

    ~WeakPtr() {
        //        if (block_) {
        //            if (block_->GetWeak() == 1) {
        //
        //            } else {
        //                block_->DecrementWeak();
        //            }
        //        }
        if (block_) {
            block_->DecrementWeak();
        }
    }

    WeakPtr& operator=(WeakPtr& other) {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncrementWeak();
            }
        }
        return *this;
    }

    template <typename Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) {
        Reset();
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            block_->IncrementWeak();
        }
        return *this;
    }

    WeakPtr<T>& operator=(const SharedPtr<T>& other) {
        if (this->ptr_ != other.ptr_) {
            Reset();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncrementWeak();
            }
        }
        return *this;
    }

    void Reset() {
        if (block_) {
            block_->DecrementWeak();
            block_ = nullptr;
        }
    }

    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    size_t UseCount() const {
        if (block_) {
            return block_->GetStrong();
        } else {
            return 0;
        }
    }

    bool Expired() const {
        return !UseCount();
    }

    SharedPtr<T> Lock() const {
        if (Expired()) {
            return SharedPtr<T>();
        } else {
            return SharedPtr<T>(WeakPtr<T>(*this));
        }
    }

private:
    T* ptr_;
    ControlBlockBase* block_;

    template <typename U>
    friend class SharedPtr;

    friend class ControlBlockBase;
};
