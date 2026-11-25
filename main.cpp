#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------
// 1. Singly linked list
// ---------------------------------------------------------------
struct Node {
    int value;
    Node* next;

    explicit Node(int val) : value(val), next(nullptr) {}
};

class LinkedList {
private:
    size_t _size;

public:
    Node* head;
    Node* tail;

    LinkedList() : _size(0), head(nullptr), tail(nullptr) {}

    ~LinkedList() {
        Node* cur = head;
        while (cur != nullptr) {
            Node* next_node = cur->next;
            delete cur;
            cur = next_node;
        }
    }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    Node* append(int value) {
        Node* new_node = new Node(value);
        if (head == nullptr) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
        _size++;
        return new_node;
    }

    void remove_node(Node* prev, Node* node) {
        if (node == nullptr) {
            throw std::invalid_argument("remove_node: node is nullptr");
        }
        if (prev == nullptr) {
            if (head != node) {
                throw std::invalid_argument("remove_node: prev is nullptr but node is not the head");
            }
            head = node->next;
        } else {
            if (prev->next != node) {
                throw std::invalid_argument("remove_node: prev is not the predecessor of node");
            }
            prev->next = node->next;
        }
        if (tail == node) {
            tail = prev;
        }
        delete node;
        _size--;
    }

    size_t size() const {
        return _size;
    }

    bool empty() const {
        return _size == 0;
    }

    std::vector<int> to_list() const {
        std::vector<int> out;
        out.reserve(_size);
        Node* cur = head;
        while (cur != nullptr) {
            out.push_back(cur->value);
            cur = cur->next;
        }
        return out;
    }

    friend std::ostream& operator<<(std::ostream& os, const LinkedList& ll) {
        if (ll._size == 0) {
            return os << "(empty)";
        }
        Node* cur = ll.head;
        while (cur != nullptr) {
            os << cur->value;
            if (cur->next != nullptr) {
                os << " -> ";
            }
            cur = cur->next;
        }
        return os;
    }
};

// ---------------------------------------------------------------
// 2. Merge sort (stable, O(n log n) time, O(n) extra space)
// ---------------------------------------------------------------
template <typename T>
std::vector<T> _merge(const std::vector<T>& left, const std::vector<T>& right) {
    std::vector<T> result;
    result.reserve(left.size() + right.size());
    size_t i = 0;
    size_t j = 0;

    while (i < left.size() && j < right.size()) {
        if (left[i] <= right[j]) {
            result.push_back(left[i]);
            i++;
        } else {
            result.push_back(right[j]);
            j++;
        }
    }
    while (i < left.size()) {
        result.push_back(left[i]);
        i++;
    }
    while (j < right.size()) {
        result.push_back(right[j]);
        j++;
    }
    return result;
}

template <typename T>
std::vector<T> merge_sort(const std::vector<T>& arr) {
    size_t n = arr.size();
    if (n <= 1) {
        return arr;
    }
    size_t mid = n / 2;
    std::vector<T> left_half(arr.begin(), arr.begin() + mid);
    std::vector<T> right_half(arr.begin() + mid, arr.end());

    std::vector<T> left = merge_sort(left_half);
    std::vector<T> right = merge_sort(right_half);
    return _merge(left, right);
}

// ---------------------------------------------------------------
// 3. Input validation
// ---------------------------------------------------------------
template <typename T>
constexpr bool is_valid_int() {
    return std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;
}

template <typename DiskT, typename HeadT, typename ReqT>
void validate(DiskT disk_size, HeadT head, const std::vector<ReqT>& requests, const std::string& direction) {
    if constexpr (!is_valid_int<DiskT>()) {
        throw std::invalid_argument("TypeError: disk_size must be an integer");
    } else {
        if (disk_size <= 0) {
            throw std::out_of_range("ValueError: disk_size must be positive, got " + std::to_string(disk_size));
        }
    }

    if (direction != "left" && direction != "right") {
        throw std::out_of_range("ValueError: direction must be 'left' or 'right', got '" + direction + "'");
    }

    if constexpr (!is_valid_int<HeadT>()) {
        throw std::invalid_argument("TypeError: head must be an integer");
    } else if constexpr (is_valid_int<DiskT>()) {
        if (head < 0 || static_cast<long long>(head) > static_cast<long long>(disk_size) - 1) {
            throw std::out_of_range("ValueError: head " + std::to_string(head) +
                                    " is outside the disk (0.." + std::to_string(disk_size - 1) + ")");
        }
    }

    if (requests.empty()) {
        throw std::out_of_range("ValueError: request list is empty");
    }

    if constexpr (!is_valid_int<ReqT>()) {
        throw std::invalid_argument("TypeError: request elements must be integers");
    } else if constexpr (is_valid_int<DiskT>()) {
        for (const auto& r : requests) {
            if (r < 0) {
                throw std::out_of_range("ValueError: request " + std::to_string(r) + " is negative");
            }
            if (static_cast<long long>(r) > static_cast<long long>(disk_size) - 1) {
                throw std::out_of_range("ValueError: request " + std::to_string(r) +
                                        " is outside the disk (0.." + std::to_string(disk_size - 1) + ")");
            }
        }
    }
}

// ---------------------------------------------------------------
// Self-tests
// ---------------------------------------------------------------
template <typename ExcType, typename DiskT, typename HeadT, typename ReqT>
bool _expect_error(DiskT disk_size, HeadT head, const std::vector<ReqT>& requests, const std::string& direction) {
    try {
        validate(disk_size, head, requests, direction);
    } catch (const ExcType&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

void _run_tests() {
    // --- linked list ---
    LinkedList ll;
    assert(ll.size() == 0 && ll.to_list().empty());
    for (int v : {98, 183, 37}) {
        ll.append(v);
    }
    assert((ll.to_list() == std::vector<int>{98, 183, 37}) && ll.size() == 3);
    assert(ll.tail->value == 37);

    // remove middle
    Node* prev = ll.head;
    Node* cur = ll.head->next;
    ll.remove_node(prev, cur);
    assert((ll.to_list() == std::vector<int>{98, 37}) && ll.size() == 2);

    // remove head
    ll.remove_node(nullptr, ll.head);
    assert((ll.to_list() == std::vector<int>{37}) && ll.head->value == 37 && ll.tail->value == 37);

    // remove tail (also the only node) -> empty, then append still works
    ll.remove_node(nullptr, ll.head);
    assert(ll.head == nullptr && ll.tail == nullptr && ll.size() == 0);
    ll.append(5);
    assert((ll.to_list() == std::vector<int>{5}));

    // remove real tail with a predecessor
    ll.append(6);
    ll.remove_node(ll.head, ll.tail);
    assert(ll.tail->value == 5 && (ll.to_list() == std::vector<int>{5}));
    ll.append(9);
    assert((ll.to_list() == std::vector<int>{5, 9}));

    // --- merge sort ---
    std::vector<int> data = {98, 183, 37, 122, 14, 124, 65, 67};
    assert((merge_sort(data) == std::vector<int>{14, 37, 65, 67, 98, 122, 124, 183}));
    assert((data == std::vector<int>{98, 183, 37, 122, 14, 124, 65, 67}));
    assert(merge_sort(std::vector<int>{}).empty());
    assert((merge_sort(std::vector<int>{7}) == std::vector<int>{7}));
    assert((merge_sort(std::vector<int>{5, 5, 1, 5}) == std::vector<int>{1, 5, 5, 5}));
    assert((merge_sort(std::vector<int>{9, 8, 7, 6, 5}) == std::vector<int>{5, 6, 7, 8, 9}));
    assert((merge_sort(std::vector<int>{1, 2, 3}) == std::vector<int>{1, 2, 3}));

    std::mt19937 rng(1);
    std::uniform_int_distribution<int> len_dist(0, 40);
    std::uniform_int_distribution<int> val_dist(0, 50);
    for (int t = 0; t < 200; ++t) {
        int len = len_dist(rng);
        std::vector<int> a(len);
        for (int& x : a) x = val_dist(rng);

        std::vector<int> expected = a;
        std::sort(expected.begin(), expected.end());
        assert(merge_sort(a) == expected);
    }

    // --- stability check ---
    struct Item {
        int k;
        std::string tag;
        bool operator<=(const Item& other) const { return k <= other.k; }
        bool operator<(const Item& other) const { return k < other.k; }
    };
    std::vector<Item> items = {{2, "a"}, {1, "b"}, {2, "c"}, {1, "d"}};
    std::vector<Item> out = merge_sort(items);
    std::vector<std::string> tags;
    for (const auto& i : out) tags.push_back(i.tag);
    assert((tags == std::vector<std::string>{"b", "d", "a", "c"}));

    // --- validation: normal ---
    validate(200, 53, data, "right");
    validate(200, 0, std::vector<int>{0, 199}, "left");
    validate(1, 0, std::vector<int>{0}, "right");

    // --- validation: failures ---
    assert((_expect_error<std::out_of_range>(200, 53, std::vector<int>{}, "right")));
    assert((_expect_error<std::out_of_range>(200, 53, std::vector<int>{200}, "right")));
    assert((_expect_error<std::out_of_range>(200, 53, std::vector<int>{-1}, "right")));
    assert((_expect_error<std::invalid_argument>(200, 53, std::vector<double>{1.5}, "right")));
    assert((_expect_error<std::invalid_argument>(200, 53, std::vector<std::string>{"7"}, "right")));
    assert((_expect_error<std::out_of_range>(200, 200, std::vector<int>{5}, "right")));
    assert((_expect_error<std::out_of_range>(200, -1, std::vector<int>{5}, "right")));
    assert((_expect_error<std::out_of_range>(0, 0, std::vector<int>{0}, "right")));
    assert((_expect_error<std::out_of_range>(-5, 0, std::vector<int>{0}, "right")));
    assert((_expect_error<std::out_of_range>(200, 53, std::vector<int>{5}, "up")));
    assert((_expect_error<std::invalid_argument>(200, 53, std::vector<bool>{true}, "right")));

    std::cout << "All Member 1 tests passed.\n";
}

int main() {
    _run_tests();
    return 0;
}