#pragma once

// Paste here your implementation of compressed_pair from seminar 2 to use in UniquePtr
#include <type_traits>
#include <utility>

template <typename T, typename V, bool IsTEmpty = std::is_empty_v<T> && !std::is_final_v<T>,
          bool IsVEmpty = std::is_empty_v<V> && !std::is_final_v<V>,
          bool IsSame = std::is_same_v<T, V>>
class MakePair {};

template <typename T, typename V>
class MakePair<T, V, true, false, false> : public T {  // пустой только первый, разные
public:
    MakePair() : T(), second_() {
    }

    template <typename F, typename S>
    MakePair(F&& first, S&& second) : T(std::forward<F>(first)), second_(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(const F& first, const S& second) : F(first), second_(second) {
    }

    template <typename F, typename S>
    MakePair(const F& first, S&& second) : F(first), second_(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(F&& first, const S& second) : F(std::forward<F>(first)), second_(second) {
    }

    T& GetFirst() {
        return *this;
    }
    const T& GetFirst() const {
        return *this;
    }

    V& GetSecond() {
        return second_;
    }
    const V& GetSecond() const {
        return second_;
    }

private:
    V second_;
};

template <typename T, typename V>
class MakePair<T, V, false, true, false> : public V {  // пустой только второй, разные
public:
    MakePair() : first_(), V() {
    }

    template <typename F, typename S>
    MakePair(F&& first, S&& second) : first_(std::forward<F>(first)), S(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(const F& first, const S& second) : first_(first), S(second) {
    }

    template <typename F, typename S>
    MakePair(const F& first, S&& second) : first_(first), S(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(F&& first, const S& second) : first_(std::forward<F>(first)), S(second) {
    }

    T& GetFirst() {
        return first_;
    }
    const T& GetFirst() const {
        return first_;
    }

    V& GetSecond() {
        return *this;
    }
    const V& GetSecond() const {
        return *this;
    }

private:
    T first_;
};

template <typename F, typename S>
class MakePair<F, S, true, true, false> : public F, public S {  // оба пустые и разные
public:
    MakePair() : F(), S() {
    }

    F& GetFirst() {
        return *this;
    }
    const F& GetFirst() const {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }
    const S& GetSecond() const {
        return *this;
    }

    MakePair(F&& first, S&& second) : F(std::forward<F>(first)), S(std::forward<S>(second)) {
    }

    MakePair(const F& first, const S& second) : F(first), S(second) {
    }

    MakePair(const F& first, S&& second) : F(first), S(std::forward<S>(second)) {
    }

    MakePair(F&& first, const S& second) : F(std::forward<F>(first)), S(second) {
    }
};

template <typename T, typename V>
class MakePair<T, V, false, false, false> {
public:
    MakePair() : first_(), second_() {
    }

    template <typename F, typename S>
    MakePair(F&& first, S&& second)
        : first_(std::forward<F>(first)), second_(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(const F& first, const S& second) : first_(first), second_(second) {
    }

    template <typename F, typename S>
    MakePair(const F& first, S&& second) : first_(first), second_(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(F&& first, const S& second) : first_(std::forward<F>(first)), second_(second) {
    }

    T& GetFirst() {
        return first_;
    }
    const T& GetFirst() const {
        return first_;
    }

    V& GetSecond() {
        return second_;
    }
    const V& GetSecond() const {
        return second_;
    }

private:
    T first_;
    V second_;
};  // оба разные и не пустые

template <typename T, typename V>
class MakePair<T, V, false, false, true> {
public:
    MakePair() : first_(), second_() {
    }

    template <typename F, typename S>
    MakePair(F&& first, S&& second) : first_(std::forward<F>(first)), S(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(const F& first, const S& second) : first_(first), S(second) {
    }

    template <typename F, typename S>
    MakePair(const F& first, S&& second) : first_(first), S(std::forward<F>(second)) {
    }

    template <typename F, typename S>
    MakePair(F&& first, const S& second) : first_(std::forward<F>(first)), S(second) {
    }

    T& GetFirst() {
        return first_;
    }
    const T& GetFirst() const {
        return first_;
    }

    V& GetSecond() {
        return second_;
    }
    const V& GetSecond() const {
        return second_;
    }

private:
    T first_;
    V second_;
};  // оба одинаковые непустые

template <typename T, typename V>
class MakePair<T, V, true, true, true> : public T {
public:
    MakePair() : first_(), T() {
    }

    template <typename F, typename S>
    MakePair(F&& first, S&& second) : first_(std::forward<F>(first)), S(std::forward<S>(second)) {
    }

    template <typename F, typename S>
    MakePair(const F& first, const S& second) : first_(first), S(second) {
    }

    template <typename F, typename S>
    MakePair(const F& first, S&& second) : first_(first), S(std::forward<T>(second)) {
    }

    template <typename F, typename S>
    MakePair(F&& first, const S& second) : first_(std::forward<T>(first)), S(second) {
    }

    T& GetFirst() {
        return *this;
    }

    const T& GetFirst() const {
        return *this;
    }

    T& GetSecond() {
        return *this;
    }

    const T& GetSecond() const {
        return *this;
    }

private:
    T first_;
};  // оба одинаковые пустые

template <typename F, typename S>
class CompressedPair
    : public MakePair<F, S, std::is_empty_v<F> && !std::is_final_v<F>,
                      std::is_empty_v<S> && !std::is_final_v<S>, std::is_same_v<F, S>> {
public:
    using Define = MakePair<F, S, std::is_empty_v<F> && !std::is_final_v<F>,
                            std::is_empty_v<S> && !std::is_final_v<S>, std::is_same_v<F, S>>;
    using Define ::Define;
};