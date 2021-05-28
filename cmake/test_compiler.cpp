/*
 * A simple program to check if compiler has C++14 support.
 */

#include <memory>

struct MyStruct {
    int a;
    int b;
};

int main() {
    auto a = std::make_unique<int>(1);
    auto b = std::make_unique<MyStruct>();
    return 0;
}
