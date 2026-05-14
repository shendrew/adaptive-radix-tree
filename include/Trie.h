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
    void insert(K& key, V& value) {
        underlying().insert_impl(key, value);
    }

    void update(K& key, V& value) {
        underlying().update_impl(key, value);
    }

    void erase(K& key) {
        underlying().erase_impl(key);
    }

    V* at(K& key) {
        return underlying().at_impl(key);
    }
    const V* at(K& key) const {
        return underlying().at_impl(key);
    }
};
