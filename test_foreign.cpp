#include "safety/foreign_pointer.h"
#include <iostream>

struct TestDeleter {
    void operator()(int* p) const noexcept {
        if (p) {
            std::cout << "Deleting: " << (void*)p << " value=" << *p << std::endl;
            delete p;
        }
    }
};

int main() {
    int* p1 = new int(42);
    int* p2 = new int(99);
    
    std::cout << "p1 = " << (void*)p1 << std::endl;
    std::cout << "p2 = " << (void*)p2 << std::endl;
    
    safety::ForeignPointer<int, TestDeleter> ptr(p1);
    std::cout << "Created ForeignPointer with p1\n";
    std::cout << "ptr.get() = " << (void*)ptr.get() << std::endl;
    
    std::cout << "Calling reset(p2)\n";
    ptr.reset(p2);
    std::cout << "After reset, ptr.get() = " << (void*)ptr.get() << std::endl;
    
    std::cout << "Exiting (should delete p2)\n";
    return 0;
}
