/*
===========================================================
 CLANG-TIDY CONFIGURATION (place this in .clang-tidy file)
===========================================================

Checks: >
  -*,
  safety-smartpointer-in-try,
  cppcoreguidelines-owning-memory,
  cppcoreguidelines-no-malloc,
  bugprone-exception-escape,
  cert-err58-cpp,
  concurrency-mt-unsafe

WarningsAsErrors: >
  safety-smartpointer-in-try,
  cppcoreguidelines-owning-memory,
  bugprone-exception-escape

===========================================================
 BUILD COMMAND
===========================================================

clang++ -std=c++20 -Wall -Wextra -Werror -O2 main.cpp

(Optional debug / safety)
clang++ -std=c++20 -g -fsanitize=address,undefined main.cpp

===========================================================
*/

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>

//
// =======================
// SAFETY INFRASTRUCTURE
// =======================
//

namespace safety {

// -----------------------
// SafeScope / SafeBoundary
// -----------------------

struct SafeScope {
    SafeScope() = default;
    ~SafeScope() noexcept = default;
};

struct SafeBoundary {
    SafeBoundary() = default;
    ~SafeBoundary() = default;
};

// -----------------------
// SafePointer (no throw)
// -----------------------

template <typename T>
class SafePointer {
public:
    explicit SafePointer(T* ptr = nullptr) noexcept : ptr_(ptr) {}

    SafePointer(const SafePointer&) = delete;
    SafePointer& operator=(const SafePointer&) = delete;

    SafePointer(SafePointer&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    SafePointer& operator=(SafePointer&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~SafePointer() noexcept {
        reset();
    }

    T* get() const noexcept { return ptr_; }
    bool valid() const noexcept { return ptr_ != nullptr; }

    void reset() noexcept {
        delete ptr_;
        ptr_ = nullptr;
    }

private:
    T* ptr_ = nullptr;
};

// -----------------------------
// SafeResultPointer (no throw)
// -----------------------------

template <typename T>
class SafeResultPointer {
public:
    static SafeResultPointer success(T* ptr) noexcept {
        return SafeResultPointer(ptr, true);
    }

    static SafeResultPointer failure() noexcept {
        return SafeResultPointer(nullptr, false);
    }

    bool ok() const noexcept { return ok_; }
    T* get() const noexcept { return ptr_; }

    ~SafeResultPointer() noexcept {
        delete ptr_;
    }

private:
    SafeResultPointer(T* ptr, bool ok) noexcept
        : ptr_(ptr), ok_(ok) {}

    T* ptr_ = nullptr;
    bool ok_ = false;
};

// ----------------------------------
// SmartPointer (throws on creation)
// ----------------------------------

template <typename T, T* (*Create)(), void (*Destroy)(T*)>
class SmartPointer {
public:
    SmartPointer() {
        ptr_ = Create();
        if (!ptr_) {
            throw std::runtime_error("SmartPointer creation failed");
        }
    }

    SmartPointer(const SmartPointer&) = delete;
    SmartPointer& operator=(const SmartPointer&) = delete;

    SmartPointer(SmartPointer&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ~SmartPointer() noexcept {
        if (ptr_) {
            Destroy(ptr_);
        }
    }

    T* get() const noexcept { return ptr_; }

private:
    T* ptr_ = nullptr;
};

} // namespace safety

//
// =======================
// SAMPLE RESOURCE
// =======================
//

struct SampleResource {
    int value = 123;
};

SampleResource* createSampleResource() {
    std::cout << "[create] SampleResource\n";
    return new SampleResource();
}

void destroySampleResource(SampleResource* r) {
    std::cout << "[destroy] SampleResource\n";
    delete r;
}

//
// =======================
// WORKER THREAD (NO-THROW)
// =======================
//

void workerThread(std::atomic<bool>& running) {
    safety::SafeScope scope; // NO exceptions allowed here

    while (running.load()) {
        safety::SafePointer<int> value(new int(42));
        std::cout << "[worker] value=" << *value.get() << "\n";
        running.store(false);
    }
}

//
// =======================
// MAIN (EXCEPTION BOUNDARY)
// =======================
//

int main() {
    safety::SafeBoundary boundary; // exception boundary

    try {
        std::cout << "=== SmartPointer demo ===\n";
        safety::SmartPointer<
            SampleResource,
            createSampleResource,
            destroySampleResource
        > resource;

        std::cout << "SampleResource value=" << resource.get()->value << "\n";

        std::cout << "\n=== SafeResultPointer demo ===\n";
        auto result = safety::SafeResultPointer<int>::success(new int(99));
        if (result.ok()) {
            std::cout << "SafeResultPointer value=" << *result.get() << "\n";
        }

        std::cout << "\n=== Worker thread demo ===\n";
        std::atomic<bool> running{true};
        std::thread t(workerThread, std::ref(running));
        t.join();

        std::cout << "\n=== Program completed safely ===\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
