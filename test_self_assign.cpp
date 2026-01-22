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
    
    std::cout << "p1 = " << (void*)p1 << std::endl;
    
    safety::ForeignPointer<int, TestDeleter> ptr(p1);
    std::cout << "Created ForeignPointer with p1\n";
    std::cout << "ptr.get() = " << (void*)ptr.get() << std::endl;
    
    std::cout << "Calling reset(p1) - self assignment\n";
    ptr.reset(p1);
    std::cout << "After reset, ptr.get() = " << (void*)ptr.get() << std::endl;
    std::cout << "Value = " << *ptr.get() << std::endl;
    
    std::cout << "Exiting (should delete p1 once)\n";
    return 0;
}
