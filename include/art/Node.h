#pragma once

#include <stdint.h>
#include <cstddef>

namespace ART {
    // node types
    static constexpr uint8_t NODE4 = 1;
    static constexpr uint8_t NODE16 = 2;
    static constexpr uint8_t NODE48 = 3;
    static constexpr uint8_t NODE256 = 4;

    // align nodes to 16 bytes for better cache performance
    struct alignas(64) Node {
        uint8_t type;
        uint8_t numChildren;
        uint16_t prefixLen;
    };

    // store key and child in same indices
    struct Node4 : public Node {
        uint8_t keys[4];
        Node *children[4];
    };

    // same as Node4
    struct Node16 : public Node {
        uint8_t keys[16];
        Node *children[16];
    };

    // directly map byte to child index
    // 1 indexed, 0 means no child
    struct Node48 : public Node {
        uint8_t indices[256];
        Node *children[48];
    };

    // array of 256 children, indexed by byte
    struct Node256 : public Node {
        Node *children[256];
    };
}