#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"

#include <cstdint>
#include <iostream>

int main() {
    ART::AdaptiveRadixTree<Encoding<uint64_t>, int> tree;

    Encoding<uint64_t> key1(1ULL);
    Encoding<uint64_t> key2(2ULL);
    Encoding<uint64_t> key3(3ULL);;
    Encoding<uint64_t> key4(4ULL);;
    Encoding<uint64_t> key5(5ULL);;

    int value1 = 10;
    int value2 = 20;
    int value3 = 30;
    int value4 = 40;
    int value5 = 50;

    tree.insert(key1, value1);
    tree.insert(key2, value2);
    tree.insert(key3, value3);
    tree.insert(key4, value4);
    tree.insert(key5, value5);


    std::cout << "Value for key1: " << *(tree.at(key1)) << std::endl;
    std::cout << "Value for key2: " << *(tree.at(key2)) << std::endl;
    std::cout << "Value for key3: " << *(tree.at(key3)) << std::endl;
    std::cout << "Value for key4: " << *(tree.at(key4)) << std::endl;
    std::cout << "Value for key5: " << *(tree.at(key5)) << std::endl;

    tree.print_info();

    return 0;
}