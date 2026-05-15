#pragma once

#include <cstddef>
#include <array>
#include <bit>

template <typename K>
struct alignas(64) Encoding {
    std::array<uint8_t, sizeof(K)> bytes{};

    constexpr size_t size() const {
        return sizeof(K);
    }

    template <typename T, typename... Args>
    constexpr void pack(int offset, T value, Args... args) {
        static_assert(std::is_trivial_v<T>, "Encoding only supports trivially copyable types");

        // byte swap integers from little to big endian for ordering
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::endian::native == std::endian::little) {
                value = std::byteswap(value);
            }
        }

        // cast to byte array and copy to struct
        auto temp = std::bit_cast<std::array<uint8_t, sizeof(T)>>(value);
        for (size_t i = 0; i < sizeof(T); i++) {
            bytes[offset + i] = temp[i];
        }

        if constexpr (sizeof...(Args) > 0) {
            pack(offset + sizeof(T), args...);
        }
    }

    constexpr Encoding() = default;  // alloc zero init with {}
    
    template <typename... Args>
    constexpr explicit Encoding(Args... args) {
        pack(0,args...);
    }

    uint8_t operator[](size_t i) const {
        return bytes[i];
    }

    bool operator==(const Encoding&) const = default;
};
