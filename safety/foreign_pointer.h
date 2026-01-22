#pragma once
#include <utility>

namespace safety {

template <typename T, typename Deleter>
class ForeignPointer {
public:
    ForeignPointer() noexcept : ptr_(nullptr), deleter_() {}

    explicit ForeignPointer(T* ptr, Deleter deleter = Deleter{}) noexcept
        : ptr_(ptr), deleter_(deleter) {}

    ~ForeignPointer() noexcept {
        reset();
    }

    ForeignPointer(const ForeignPointer&) = delete;
    ForeignPointer& operator=(const ForeignPointer&) = delete;

    ForeignPointer(ForeignPointer&& other) noexcept
        : ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
        other.ptr_ = nullptr;
    }

    ForeignPointer& operator=(ForeignPointer&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            deleter_ = std::move(other.deleter_);
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    void reset(T* ptr = nullptr) noexcept {
        if (ptr_) {
            deleter_(ptr_);
        }
        ptr_ = ptr;
    }

private:
    T* ptr_;
    Deleter deleter_;
};

} // namespace safety
