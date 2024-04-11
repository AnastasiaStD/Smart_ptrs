#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>
#include <utility>

template <typename T>
struct Slug {
    Slug() = default;

    template <typename U>
    Slug(const Slug<U>&) {
    }

    void operator()(T* ptr) const {
        static_assert(!std::is_void_v<T>);
        static_assert(sizeof(T) > 0);
        delete ptr;
    }
};

template <>
struct Slug<void> {
    void operator()(void* p) {
        int* x = reinterpret_cast<int*>(p);
        delete x;
    }
};

template <typename T>
struct Slug<T[]> {
    void operator()(T* ptr) const {
        delete[] ptr;
    }
};

// Primary template
template <class T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) noexcept : object_(ptr, Deleter()){};

    UniquePtr(T* ptr, Deleter deleter) {
        object_ = CompressedPair<T*, Deleter>(ptr, std::forward<Deleter>(deleter));
    };

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept {
        object_ = CompressedPair<T*, Deleter>(static_cast<T*>(other.Release()),
                                              std::forward<Deleter>(other.GetDeleter()));
    };

    template <class U, class OtherDeleter = Slug<U>>
    UniquePtr(UniquePtr<U, OtherDeleter>&& other) noexcept {
        // object_ = CompressedPair((other.Release()),
        // std::forward<OtherDeleter>(other.GetDeleter()));
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::forward<OtherDeleter>(other.GetDeleter());
    };

    //    template <class U>
    //    explicit UniquePtr(UniquePtr<U>&& other) noexcept
    //        : object_((CompressedPair<U*, Slug<U*>>((std::forward<U*>(other.Release())),
    //                                                std::forward<Slug<U*>>(other.GetDeleter())))){};

    //    : object_(
    //         (CompressedPair<U*, OtherDeleter>((std::forward<U*>(other.Release())),
    //                                           std::forward<OtherDeleter>(other.GetDeleter()))))

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    auto&& operator=(const UniquePtr& other) = delete;
    auto&& operator=(UniquePtr&& other) noexcept {
        if (other.Get() != this->object_.GetFirst()) {
            Reset(other.Release());
            object_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
        }
        return *this;
    };

    template <class U, class OtherDeleter = Slug<U>>
    auto&& operator=(UniquePtr<U, OtherDeleter>&& other) noexcept {
        if (other.Get() != this->object_.GetFirst()) {
            auto temp = other.Release();
            Reset(temp);
            object_.GetSecond() = std::forward<OtherDeleter>(other.GetDeleter());
        }
        return *this;
    };

    auto&& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() noexcept {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        auto tmp = object_.GetFirst();
        object_.GetFirst() = nullptr;
        return tmp;
    };
    void Reset(T* ptr = nullptr) noexcept {
        if (ptr != object_.GetFirst()) {
            auto old_ptr = object_.GetFirst();
            object_.GetFirst() = ptr;
            object_.GetSecond()(old_ptr);
        }
    };

    void Swap(UniquePtr& other) noexcept {
        std::swap(object_.GetFirst(), other.object_.GetFirst());
        std::swap(object_.GetSecond(), other.object_.GetSecond());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const noexcept {
        return object_.GetFirst();
    };
    Deleter& GetDeleter() {
        return object_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return object_.GetSecond();
    };
    explicit operator bool() const noexcept {
        return (object_.GetFirst() != nullptr);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T operator*() const {
        return *object_.GetFirst();
    };
    T* operator->() const noexcept {
        return object_.GetFirst();
    };

private:
    CompressedPair<T*, Deleter> object_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) noexcept : object_({ptr, Deleter()}){};
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr(T* ptr, Deleter deleter) : object_({ptr, std::move(deleter)}) {
        object_.GetSecond() = std::move(deleter);
    };

    UniquePtr(UniquePtr&& other) noexcept
        : object_({std::move(other.object_.GetFirst()), std::move(other.object_.GetSecond())}) {
        other.Reset();
    };

    template <class U, class OtherDeleter = Slug<U>>
    UniquePtr(UniquePtr<U, OtherDeleter>&& other) noexcept {
        // object_ = CompressedPair((other.Release()),
        // std::forward<OtherDeleter>(other.GetDeleter()));
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::forward<OtherDeleter>(other.GetDeleter());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    UniquePtr& operator=(const UniquePtr& rhs) = delete;
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            Reset(other.Release());
            object_.GetSecond() = std::move(other.object_.GetSecond());
        }
        return *this;
    };

    template <class U, class OtherDeleter = Slug<U>>
    auto&& operator=(UniquePtr<U, OtherDeleter>&& other) noexcept {
        if (other.Get() != this->object_.GetFirst()) {
            auto temp = other.Release();
            Reset(temp);
            object_.GetSecond() = std::forward<OtherDeleter>(other.GetDeleter());
        }
        return *this;
    };
    UniquePtr& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() noexcept {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        T* tmp = object_.GetFirst();
        object_.GetFirst() = nullptr;
        return tmp;
    };
    void Reset(T* ptr = nullptr) {
        if (object_.GetFirst() != ptr) {
            T* old_ptr = object_.GetFirst();
            object_.GetFirst() = ptr;
            object_.GetFirst() = ptr;
            object_.GetSecond()(old_ptr);
        }
    };

    void Swap(UniquePtr& other) noexcept {
        std::swap(object_.GetFirst(), other.object_.GetFirst());
        std::swap(object_.GetSecond(), other.object_.GetSecond());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return object_.GetFirst();
    };
    Deleter& GetDeleter() {
        return object_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return object_.GetSecond();
    };
    explicit operator bool() const {
        return object_.GetFirst() != nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Array dereference operators

    T& operator[](size_t index) const {
        return object_.GetFirst()[index];
    };

private:
    using PairType = CompressedPair<T*, Deleter>;
    PairType object_;
};

template <typename T1, typename D1, typename T2, typename D2>
bool operator==(const UniquePtr<T1, D1>& p1, const UniquePtr<T2, D2>& p2) {
    return p1.Get() == p2.Get();
}

template <typename T1, typename D1, typename T2, typename D2>
bool operator!=(const UniquePtr<T1, D1>& p1, const UniquePtr<T2, D2>& p2) {
    return p1.Get() != p2.Get();
}

template <typename T, typename D>
bool operator==(const UniquePtr<T, D>& p, std::nullptr_t) noexcept {
    return p.Get() == nullptr;
}

template <typename T, typename D>
bool operator==(std::nullptr_t, const UniquePtr<T, D>& p) noexcept {
    return nullptr == p.Get();
}

template <typename T, typename D>
bool operator!=(const UniquePtr<T, D>& p, std::nullptr_t) noexcept {
    return p.Get() != nullptr;
}

template <typename T, typename D>
bool operator!=(std::nullptr_t, const UniquePtr<T, D>& p) noexcept {
    return nullptr != p.Get();
}
