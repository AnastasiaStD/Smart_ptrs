#pragma once

#include <exception>
#include <cstddef>

class BadWeakPtr : public std::exception {};

class ControlBlockBase {
public:
    virtual void ZeroStrong() = 0;
    virtual void ZeroWeak() = 0;
    virtual ~ControlBlockBase() = default;

    virtual void IncrementStrong() {
        ++strong_;
    };
    virtual void DecrementStrong() {
        --strong_;
        if (strong_ == 0) {
            delete this;
        }
    };
    virtual void IncrementWeak() {
        ++weak_;
    };
    virtual void DecrementWeak() {
        --weak_;
        if (weak_ == 0) {
            delete this;
        }
    };

    virtual int GetStrong() {
        return strong_;
    }

    virtual int GetWeak() {
        return weak_;
    }

    virtual void Put() {
        strong_ = 1;
    }

    virtual bool ExistsStrong() {
        return (strong_ > 0);
    }
    virtual bool ExistsWeak() {
        return (weak_ > 0);
    }

    int strong_ = 1;
    int weak_ = 0;
};

template <typename T>
class ControlBlockPointerImpl : public ControlBlockBase {
public:
    ~ControlBlockPointerImpl() override {
        if (ptr_) {
            delete ptr_;
        }
    }

    explicit ControlBlockPointerImpl(T* p) {
        ptr_ = p;
    }

    void ZeroStrong() override {
        auto tmp = ptr_;
        ptr_ = nullptr;
        delete tmp;
    }

    void ZeroWeak() override {
        delete this;
    }

private:
    T* ptr_ = nullptr;
};

template <typename T>
class ControlBlockEmplaceImpl : public ControlBlockBase {
public:
    template <typename... Args>
    explicit ControlBlockEmplaceImpl(Args&&... args) {
        new (&holder_) T(std::forward<Args>(args)...);
    }

    ~ControlBlockEmplaceImpl() override {
        GetRawPtr()->~T();
    }

    void ZeroStrong() override {
        GetRawPtr()->~T();
    }

    void ZeroWeak() override {
        delete this;
    }

    T* GetRawPtr() {
        return reinterpret_cast<T*>(&holder_);
    }

private:
    alignas(T) unsigned char holder_[sizeof(T)];
};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr {
public:
    WeakPtr() = default;
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
    WeakPtr(const WeakPtr<Y>&& other) : ptr_(other.ptr_), block_(std::move(other.block_)) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    WeakPtr(const SharedPtr<T>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncrementWeak();
        }
    }

    ~WeakPtr() {
        Reset();
    }

    WeakPtr& operator=(const WeakPtr& other) {
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

    WeakPtr& operator=(const SharedPtr<T>& other) {
        Reset();
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            block_->IncrementWeak();
        }
        return *this;
    }

    void Reset() {
        if (block_) {
            block_->DecrementWeak();
            if (!block_->GetWeak() && !block_->GetStrong()) {
                delete block_;
            }
        }
        ptr_ = nullptr;
        block_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    size_t UseCount() const {
        return block_ ? block_->GetStrong() : 0;
    }

    bool Expired() const {
        return !UseCount();
    }

    SharedPtr<T> Lock() const {
        return Expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    }

private:
    T* ptr_;
    ControlBlockBase* block_;

    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;

    friend class ControlBlockBase;
};
