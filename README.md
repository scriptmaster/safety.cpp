# safety.cpp (ai-agents-compatible)

Write safe ai-agents-compatible code with safety.cpp framework in modern C++.

## No rust, zig, vlang or golang.

# C++ Safety Infrastructure (RAII + clang-tidy enforced)

This repository provides a **portable, drop-in safety framework for modern C++**.

It enforces:
- **Memory safety**
- **Exception safety**
- **Thread safety**
- **Clear architectural boundaries**

…using **RAII + clang-tidy**, not conventions or comments.

This repo is intentionally **project-agnostic** and can be copied into *any* C++ codebase.

---

## 🎯 Goals

- Eliminate owning raw pointers (`T*`)
- Make crashes *structurally impossible*, not “unlikely”
- Restrict exceptions to explicit boundaries
- Prevent unsafe AI-generated code from entering the codebase
- Enforce rules automatically using tooling

---

## 🧱 Core Concepts

### 1. Safe Boundaries

| Concept | Purpose |
|--------|---------|
| `SafeBoundary` | Where exceptions are allowed (startup, glue code) |
| `SafeScope` | No-throw zones (workers, realtime loops) |

```cpp
void init() {
    safety::SafeBoundary boundary;
    try {
        // allowed
    } catch (...) {
        // required
    }
}

void worker() {
    safety::SafeScope scope;
    // exceptions forbidden
}
```

---

### 2. Pointer Types

This framework defines **exactly four ownership patterns**.

#### `SafePointer<T>`
- **For application-level objects**
- No exceptions
- RAII ownership
- Used in workers, loops, realtime code

#### `SafeResultPointer<T>`
- **For application-level objects with explicit error handling**
- Go-style error handling
- Explicit success / failure
- No exceptions

#### `SmartPointer<T>`
- **For startup-only, throwing initialization**
- Throws on construction
- **ONLY allowed inside `try/catch`**
- Enforced by clang-tidy

```cpp
try {
    safety::SmartPointer<Resource, create, destroy> r;
} catch (...) {
    // mandatory
}
```

#### `ForeignPointer<T, Deleter>`
- **REQUIRED for owned C / FFI / GPU / OS handles**
- Replaces `std::unique_ptr` for foreign resources
- No exceptions
- Custom deleter for cleanup
- **DO NOT use SafePointer or SmartPointer for C API handles**

```cpp
struct FileDeleter {
    void operator()(FILE* f) const noexcept {
        if (f) fclose(f);
    }
};

safety::ForeignPointer<FILE, FileDeleter> file(fopen("data.txt", "r"));
```

---

## 🚫 What Is Explicitly Forbidden

Enforced by tooling:

- Owning raw pointers (`T*`)
- `new` / `delete` / `malloc`
- Exceptions inside worker threads
- `SmartPointer` outside `try/catch`
- Silent resource failures
- `std::unique_ptr` for C / FFI / GPU / OS handles (use `ForeignPointer` instead)
- `SafePointer` or `SmartPointer` for C API handles
- “Best effort” safety

If it compiles, it is safe by construction.

---

## 🛠 Tooling (Required)

### LLVM / Clang

Install once (Windows):

```powershell
winget install LLVM.LLVM
```

Includes:
- `clang++`
- `clang-tidy`
- `clang-check`
- sanitizers

---

### clang-tidy (Enforcement)

A `.clang-tidy` file is required at repo root.

The framework relies on:
- Custom rule: `safety-smartpointer-in-try`
- Custom rule: `safety-foreignpointer-for-c-api`
- Core C++ safety rules
- Warnings treated as **errors**

If rules are violated → **build fails**.

---

## 📁 Repository Layout

```
.
├─ safety/
│  ├─ smart_pointer.h
│  ├─ safe_scope.h
│  ├─ foreign_pointer.h
│  └─ (other safety primitives)
│
├─ tools/
│  └─ clang-tidy/
│     ├─ SmartPointerInTryCheck.h
│     └─ SmartPointerInTryCheck.cpp
│
├─ main.cpp        # reference implementation
├─ .clang-tidy
└─ README.md
```

---

## ▶️ Build & Run

```bash
clang++ -std=c++20 -Wall -Wextra -Werror -O2 main.cpp
```

Debug + sanitizers:

```bash
clang++ -std=c++20 -g -fsanitize=address,undefined main.cpp
```

---

## 🧠 Why This Exists

C++ crashes don’t happen because developers are careless.  
They happen because **the language allows unsafe states**.

This framework:
- Removes unsafe states
- Makes safety *structural*
- Makes AI-generated code safe by default
- Moves correctness from “review” to “compilation”

---

## 🤖 AI-Generated Code Compatibility

This repo is designed specifically to work with:
- GitHub Copilot
- Cursor
- LLM-based coding agents

Unsafe patterns are **rejected automatically**, not politely suggested against.

---

## 🧩 Intended Usage

You can:
- Copy `safety/` into any repo
- Copy `.clang-tidy`
- Enable clang-tidy in CI
- Refactor incrementally

No framework lock-in.  
No runtime dependency.  
No macros.

---

## 📌 Design Principles

- RAII over garbage collection
- Boundaries over global rules
- Tooling over discipline
- Determinism over cleverness
- Fewer abstractions, enforced harder

---

## 🔒 Final Rule

> **If the code compiles, it is safe by construction.**
