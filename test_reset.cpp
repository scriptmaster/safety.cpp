#include <iostream>
#include <memory>

struct Deleter {
    void operator()(int* p) const {
        std::cout << "Deleting: " << (void*)p << std::endl;
        delete p;
    }
};

int main() {
    int* p1 = new int(42);
    std::cout << "p1 = " << (void*)p1 << std::endl;
    
    std::unique_ptr<int, Deleter> ptr(p1);
    std::cout << "Before reset with same pointer\n";
    ptr.reset(p1);  // What happens here?
    std::cout << "After reset with same pointer\n";
    
    return 0;
}
