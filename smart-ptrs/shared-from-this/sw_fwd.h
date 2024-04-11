#pragma once

#include <exception>
#include <cstddef>

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

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
        if (strong_ == 0 && weak_ != 0) {
            ZeroStrong();
        } else if (weak_ == 0 && strong_ == 0) {
            ZeroStrong();
            ZeroWeak();
        }
    };
    virtual void IncrementWeak() {
        ++weak_;
    };
    virtual void DecrementWeak() {
        --weak_;
        if (weak_ + strong_ == 0) {
            ZeroWeak();
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

    virtual void PutWeak() {
        weak_ = 0;
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
    ~ControlBlockPointerImpl() override = default;

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

    ~ControlBlockEmplaceImpl() override = default;
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
