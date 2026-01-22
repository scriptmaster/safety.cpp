/*
===========================================================
 CLANG-TIDY CONFIGURATION (place this in .clang-tidy file)
===========================================================

Checks: >
  -*,
  safety-smartpointer-in-try,
  safety-foreignpointer-for-c-api,
  cppcoreguidelines-owning-memory,
  cppcoreguidelines-no-malloc,
  bugprone-exception-escape,
  cert-err58-cpp,
  concurrency-mt-unsafe

WarningsAsErrors: >
  safety-smartpointer-in-try,
  safety-foreignpointer-for-c-api,
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
#include <cstdio>
#include <cstdlib>

// Include the ForeignPointer header
#include "safety/foreign_pointer.h"

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
// C API RESOURCE DEMO
// =======================
//

// Example deleter for FILE* (C API handle)
struct FileDeleter {
    void operator()(FILE* f) const noexcept {
        if (f) {
            std::cout << "[close] FILE handle\n";
            fclose(f);
        }
    }
};

// Example deleter for a hypothetical C API context
struct CApiContextDeleter {
    void operator()(void* ctx) const noexcept {
        if (ctx) {
            std::cout << "[free] C API context\n";
            // In a real scenario: c_api_destroy(ctx);
            free(ctx);
        }
    }
};

//
// =======================
// WORKER THREAD (NO-THROW)
// =======================
//

void workerThread(std::atomic<bool>& running) {
    safety::SafeScope scope; // NO exceptions allowed here
    (void)scope; // Mark as intentionally unused

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
    (void)boundary; // Mark as intentionally unused

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

        std::cout << "\n=== ForeignPointer demo (C API handles) ===\n";
        // Demo 1: FILE* handle using ForeignPointer
        {
            safety::ForeignPointer<FILE, FileDeleter> file(
                fopen("/tmp/test_safety.txt", "w")
            );
            if (file) {
                std::cout << "FILE* opened successfully\n";
                fprintf(file.get(), "ForeignPointer test\n");
                // file automatically closed when going out of scope
            }
        }

        // Demo 2: C API context (simulated)
        {
            void* ctx = malloc(64); // Simulating c_api_create()
            safety::ForeignPointer<void, CApiContextDeleter> context(ctx);
            if (context) {
                std::cout << "C API context created successfully\n";
                // context automatically freed when going out of scope
            }
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
