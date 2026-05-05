#pragma once

#include <stdint.h>

struct NodeHeader {

};

// store key and child in same indices
struct Node4 {
    NodeHeader header;
    uint8_t keys[4];
    void *children[4];
};

// same as Node4
struct Node16 {
    NodeHeader header;
    uint8_t keys[16];
    void *children[16];
};

// directly map byte to child index
struct Node48 {
    NodeHeader header;
    uint8_t indices[256];
    void *children[48];
};

// array of 256 children, indexed by byte
struct Node256 {
    NodeHeader header;
    void *children[256];
};