#pragma once

template <typename T, typename K, typename V>
class Trie {
private:
    T& underlying() {
        return static_cast<T&>(*this);
    }
    const T& underlying() const {
        return static_cast<const T&>(*this);
    }

public:
    void insert(K&& key, V&& value) {
        underlying().insert_impl(std::forward<K>(key), std::forward<V>(value));
    }

    void erase(K&& key) {
        underlying().erase_impl(std::forward<K>(key));
    }

    V* at(K&& key) {
        return underlying().find_impl(std::forward<K>(key));
    }
    const V* at(K&& key) const {
        return underlying().find_impl(std::forward<K>(key));
    }
};
