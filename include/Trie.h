#pragma once

template <typename T>
class Trie {
private:
    T& underlying() {
        return static_cast<T&>(*this);
    }
    const T& underlying() const {
        return static_cast<const T&>(*this);
    }

public:
    template <typename K, typename V>
    void insert(K&& key, V&& value) {
        underlying().insert_impl(std::forward<K>(key), std::forward<V>(value));
    }

    template <typename K>
    void erase(K&& key) {
        underlying().erase_impl(std::forward<K>(key));
    }

    template <typename K>
    K* at(K&& key) {
        return underlying().find_impl(std::forward<K>(key));
    }
    template <typename K>
    const K* at(K&& key) const {
        return underlying().find_impl(std::forward<K>(key));
    }
};
