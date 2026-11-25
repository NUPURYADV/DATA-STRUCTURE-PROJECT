// ============================================================================
// Disk Request Scheduling & Head Movement Simulator   (C++17)
// Data Structures Lab - Technical Mini Project 07
//
// Build : g++ -std=c++17 -O2 -o disk_scheduler disk_scheduler.cpp
// Run   : ./disk_scheduler demo               textbook example, all 5 algorithms
//         ./disk_scheduler test               full test suite (normal/boundary/failure)
//         ./disk_scheduler experiment         experiments (sizes 10, 100, 1000)
//         ./disk_scheduler experiment --big   also size 10000 (SSTF is O(n^2), slow)
//         ./disk_scheduler run                type your own input
//
// CONVENTIONS (write these in the report)
//  1. Tracks are 0 .. disk_size-1.
//  2. Total head movement = sum of |next - current| starting from the head.
//  3. SCAN travels to the physical end of the disk before reversing.
//  4. C-SCAN counts the return jump as movement. If there is nothing left to
//     serve on the far side, the jump is not made.
//  5. LOOK goes only as far as the last request in each direction.
//  6. Average seek = total movement / number of REAL requests (virtual end
//     stops such as 0 or disk_size-1 are not counted in the denominator).
//  7. SSTF tie-break: if two requests are at equal distance, the LOWER track wins.
//  8. A request equal to the head is served at zero cost, first in the sweep.
//
// Only our own LinkedList and merge sort are used as core structures.
// (std::vector is used as the plain "array"; no std::sort / deque / heap.)
// ============================================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>

using namespace std;

// ============================================================================
// SECTION 1 (Member 1): Linked list, merge sort, validation
// ============================================================================
struct Node {
    int value;
    Node* next;
    explicit Node(int v) : value(v), next(nullptr) {}
};

// Singly linked list with a tail pointer.
//   append O(1) | removeNode(prev,node) O(1) | traversal O(n) | size O(1)
class LinkedList {
    Node* head_;
    Node* tail_;
    int size_;

public:
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~LinkedList() { clear(); }
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    LinkedList(LinkedList&& o) noexcept : head_(o.head_), tail_(o.tail_), size_(o.size_) {
        o.head_ = o.tail_ = nullptr;
        o.size_ = 0;
    }
    LinkedList& operator=(LinkedList&& o) noexcept {
        if (this != &o) {
            clear();
            head_ = o.head_; tail_ = o.tail_; size_ = o.size_;
            o.head_ = o.tail_ = nullptr;
            o.size_ = 0;
        }
        return *this;
    }

    void append(int v) {
        Node* n = new Node(v);
        if (head_ == nullptr) head_ = tail_ = n;   // empty list
        else { tail_->next = n; tail_ = n; }       // link after tail, move tail
        size_++;
    }

    // Removes (and deletes) `node`. `prev` is its predecessor, or nullptr if node is the head.
    void removeNode(Node* prev, Node* node) {
        if (node == nullptr) throw logic_error("removeNode: node is null");
        if (prev == nullptr) {
            if (head_ != node) throw logic_error("removeNode: prev is null but node is not head");
            head_ = node->next;
        } else {
            if (prev->next != node) throw logic_error("removeNode: prev is not predecessor");
            prev->next = node->next;
        }
        if (tail_ == node) tail_ = prev;
        delete node;
        size_--;
    }

    Node* head() const { return head_; }
    Node* tail() const { return tail_; }
    int size() const { return size_; }

    vector<int> toVector() const {
        vector<int> out;
        for (Node* p = head_; p != nullptr; p = p->next) out.push_back(p->value);
        return out;
    }

    void clear() {
        Node* p = head_;
        while (p != nullptr) { Node* nx = p->next; delete p; p = nx; }
        head_ = tail_ = nullptr;
        size_ = 0;
    }
};

// Merge sort: stable, O(n log n) time, O(n) extra space. Returns a new sorted vector.
static vector<int> mergeTwo(const vector<int>& a, const vector<int>& b) {
    vector<int> r;
    r.reserve(a.size() + b.size());
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] <= b[j]) r.push_back(a[i++]);   // <= keeps equal values in original order (stable)
        else r.push_back(b[j++]);
    }
    while (i < a.size()) r.push_back(a[i++]);
    while (j < b.size()) r.push_back(b[j++]);
    return r;
}

vector<int> mergeSort(const vector<int>& v) {
    if (v.size() <= 1) return v;
    size_t mid = v.size() / 2;
    vector<int> left(v.begin(), v.begin() + mid);
    vector<int> right(v.begin() + mid, v.end());
    return mergeTwo(mergeSort(left), mergeSort(right));
}

// Parse a token into an int; rejects "3.5", "abc", "", overflow, etc.
int parseInt(const string& s) {
    if (s.empty()) throw invalid_argument("empty value is not an integer");
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) throw invalid_argument("'" + s + "' is not an integer");
    for (size_t k = i; k < s.size(); k++)
        if (s[k] < '0' || s[k] > '9') throw invalid_argument("'" + s + "' is not an integer");
    if (s.size() - i > 9) throw invalid_argument("'" + s + "' is too large");
    return atoi(s.c_str());
}

// Throws invalid_argument with a clear message for any invalid input.
void validate(int diskSize, int head, const vector<int>& requests, const string& direction) {
    if (diskSize <= 0) throw invalid_argument("disk size must be positive");
    if (direction != "left" && direction != "right")
        throw invalid_argument("direction must be 'left' or 'right'");
    if (head < 0 || head > diskSize - 1)
        throw invalid_argument("head " + to_string(head) + " is outside the disk");
    if (requests.empty()) throw invalid_argument("request list is empty");
    for (size_t i = 0; i < requests.size(); i++) {
        int r = requests[i];
        if (r < 0) throw invalid_argument("request " + to_string(r) + " is negative");
        if (r > diskSize - 1)
            throw invalid_argument("request " + to_string(r) + " is outside the disk");
    }
}

// ============================================================================
// Result type and shared metric (Member 5)
// ============================================================================
struct Result {
    LinkedList order;     // full service sequence (starting AFTER the head position)
    vector<int> virt;     // virtual stops (e.g. 199 or 0) the head visits but nobody requested
};

typedef Result (*Algo)(const vector<int>&, int, int, const string&);

// One shared function used by every algorithm to measure movement.
long long totalMovement(const LinkedList& order, int head) {
    long long total = 0;
    int cur = head;
    for (Node* p = order.head(); p != nullptr; p = p->next) {
        total += abs(p->value - cur);
        cur = p->value;
    }
    return total;
}

// ============================================================================
// SECTION 2 (Member 2): FCFS and SSTF
// ============================================================================
// FCFS: serve in arrival order (a FIFO queue idea). O(n) time, O(n) space.
Result fcfs(const vector<int>& req, int head, int diskSize, const string& dir) {
    validate(diskSize, head, req, dir);
    Result r;
    for (size_t i = 0; i < req.size(); i++) r.order.append(req[i]);
    return r;
}

// SSTF: repeatedly pick the pending request nearest to the head.
// O(n^2) time (n searches of an O(n) list), O(n) space. Can starve far requests.
Result sstf(const vector<int>& req, int head, int diskSize, const string& dir) {
    validate(diskSize, head, req, dir);
    Result r;
    LinkedList pending;
    for (size_t i = 0; i < req.size(); i++) pending.append(req[i]);

    int cur = head;
    while (pending.size() > 0) {
        Node* best = nullptr;
        Node* bestPrev = nullptr;
        int bestDist = 0;
        Node* prev = nullptr;
        for (Node* p = pending.head(); p != nullptr; prev = p, p = p->next) {
            int d = abs(p->value - cur);
            // strictly closer wins; on a tie the LOWER track wins
            if (best == nullptr || d < bestDist || (d == bestDist && p->value < best->value)) {
                best = p; bestPrev = prev; bestDist = d;
            }
        }
        r.order.append(best->value);
        cur = best->value;
        pending.removeNode(bestPrev, best);
    }
    return r;
}

// ============================================================================
// SECTION 3 (Member 3): SCAN, C-SCAN, LOOK
// ============================================================================
// Split a sorted array around the head. Both outputs stay ascending.
//   direction right: lower = tracks <  head, upper = tracks >= head
//   direction left : lower = tracks <= head, upper = tracks >  head
// (a request equal to the head is served first, at zero cost, in either direction)
static void _split(const vector<int>& sorted, int head, const string& dir,
                   vector<int>& lower, vector<int>& upper) {
    for (size_t i = 0; i < sorted.size(); i++) {
        int v = sorted[i];
        bool isLower = (dir == "right") ? (v < head) : (v <= head);
        if (isLower) lower.push_back(v);
        else upper.push_back(v);
    }
}

static void addAsc(Result& r, const vector<int>& v) {
    for (size_t i = 0; i < v.size(); i++) r.order.append(v[i]);
}
static void addDesc(Result& r, const vector<int>& v) {
    for (size_t i = v.size(); i > 0; i--) r.order.append(v[i - 1]);
}
static void addVirtual(Result& r, int track) {
    r.order.append(track);
    r.virt.push_back(track);
}

// SCAN (elevator): sweep to the PHYSICAL end, reverse, sweep back.
Result scan(const vector<int>& req, int head, int diskSize, const string& dir) {
    validate(diskSize, head, req, dir);
    vector<int> sorted = mergeSort(req), lower, upper;
    _split(sorted, head, dir, lower, upper);
    Result r;
    int end = diskSize - 1;
    if (dir == "right") {
        addAsc(r, upper);
        int pos = upper.empty() ? head : upper.back();
        if (pos != end) addVirtual(r, end);          // travel to the physical end
        addDesc(r, lower);
    } else {
        addDesc(r, lower);
        int pos = lower.empty() ? head : lower[0];
        if (pos != 0) addVirtual(r, 0);              // travel to track 0
        addAsc(r, upper);
    }
    return r;
}

// C-SCAN: sweep to the end, JUMP to the other end (counted), sweep the same way again.
Result cscan(const vector<int>& req, int head, int diskSize, const string& dir) {
    validate(diskSize, head, req, dir);
    vector<int> sorted = mergeSort(req), lower, upper;
    _split(sorted, head, dir, lower, upper);
    Result r;
    int end = diskSize - 1;
    if (dir == "right") {
        addAsc(r, upper);
        int pos = upper.empty() ? head : upper.back();
        if (pos != end) addVirtual(r, end);
        if (!lower.empty()) {                        // jump only if something is left to serve
            if (lower[0] != 0) addVirtual(r, 0);     // landing on 0 (jump counted as movement)
            addAsc(r, lower);                        // lower part ASCENDING (not descending)
        }
    } else {
        addDesc(r, lower);
        int pos = lower.empty() ? head : lower[0];
        if (pos != 0) addVirtual(r, 0);
        if (!upper.empty()) {
            if (upper.back() != end) addVirtual(r, end);
            addDesc(r, upper);                       // upper part DESCENDING
        }
    }
    return r;
}

// LOOK: like SCAN, but turns around at the last request, not at the disk end.
Result look(const vector<int>& req, int head, int diskSize, const string& dir) {
    validate(diskSize, head, req, dir);
    vector<int> sorted = mergeSort(req), lower, upper;
    _split(sorted, head, dir, lower, upper);
    Result r;
    if (dir == "right") { addAsc(r, upper); addDesc(r, lower); }
    else                { addDesc(r, lower); addAsc(r, upper); }
    return r;
}

// ============================================================================
// SECTION 4 (Member 5): run_algorithm, compare, path export
// ============================================================================
struct AlgoInfo { const char* name; Algo fn; };
static const AlgoInfo ALGOS[5] = {
    {"FCFS", fcfs}, {"SSTF", sstf}, {"SCAN", scan}, {"C-SCAN", cscan}, {"LOOK", look}
};

struct Row {
    string name;
    long long total;
    double avgSeek;
    vector<int> order;
    vector<int> virt;
};

Row runAlgorithm(const AlgoInfo& a, const vector<int>& req, int head, int diskSize,
                 const string& dir) {
    Result res = a.fn(req, head, diskSize, dir);
    Row row;
    row.name = a.name;
    row.total = totalMovement(res.order, head);
    row.avgSeek = (double)row.total / (double)req.size();   // real requests only
    row.order = res.order.toVector();
    row.virt = res.virt;
    return row;
}

void saveAllPaths(const vector<Row>& rows, int head) {
    for (size_t k = 0; k < rows.size(); k++) {
        ofstream f("path_" + rows[k].name + ".csv");
        f << "step,track\n0," << head << "\n";
        for (size_t i = 0; i < rows[k].order.size(); i++)
            f << (i + 1) << "," << rows[k].order[i] << "\n";
    }
}

static bool isVirtual(const Row& r, int track) {
    for (size_t i = 0; i < r.virt.size(); i++) if (r.virt[i] == track) return true;
    return false;
}

// Runs all five on the IDENTICAL input and prints a table sorted by total movement.
vector<Row> compare(const vector<int>& req, int head, int diskSize, const string& dir,
                    bool showOrder) {
    vector<Row> rows;
    for (int i = 0; i < 5; i++) rows.push_back(runAlgorithm(ALGOS[i], req, head, diskSize, dir));

    // insertion sort by total movement (our own, no std::sort)
    vector<Row> sorted = rows;
    for (size_t i = 1; i < sorted.size(); i++) {
        Row key = sorted[i];
        int j = (int)i - 1;
        while (j >= 0 && sorted[j].total > key.total) { sorted[j + 1] = sorted[j]; j--; }
        sorted[j + 1] = key;
    }

    cout << "Disk 0-" << diskSize - 1 << ", head " << head << ", direction " << dir
         << ", " << req.size() << " requests\n";
    cout << left << setw(8) << "Algo" << right << setw(10) << "Total"
         << setw(12) << "Avg seek" << "   Service order (* = virtual stop)\n";
    cout << string(70, '-') << "\n";
    for (size_t i = 0; i < sorted.size(); i++) {
        cout << left << setw(8) << sorted[i].name << right << setw(10) << sorted[i].total
             << setw(12) << fixed << setprecision(2) << sorted[i].avgSeek << "   ";
        if (showOrder && sorted[i].order.size() <= 25) {
            for (size_t k = 0; k < sorted[i].order.size(); k++) {
                int t = sorted[i].order[k];
                cout << t << (isVirtual(sorted[i], t) ? "*" : "") << " ";
            }
        }
        cout << "\n";
    }
    return rows;   // original algorithm order
}

// ============================================================================
// SECTION 5 (Member 4): workload generator, tests, experiments
// ============================================================================
// Own small PRNG (xorshift64*) so results are identical on every machine for a given seed.
struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed * 2654435761ULL + 88172645463325252ULL) {}
    uint64_t next() {
        s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
        return s * 2685821657736338717ULL;
    }
    int range(int lo, int hi) { return lo + (int)(next() % (uint64_t)(hi - lo + 1)); }
};

static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// types: "random", "clustered", "sequential", "skewed"
vector<int> generateWorkload(const string& type, int n, int diskSize, uint64_t seed) {
    Rng rng(seed);
    vector<int> out;
    if (type == "random") {
        for (int i = 0; i < n; i++) out.push_back(rng.range(0, diskSize - 1));
    } else if (type == "clustered") {                 // around one hotspot
        int center = rng.range(0, diskSize - 1);
        int spread = diskSize / 20 > 1 ? diskSize / 20 : 1;
        for (int i = 0; i < n; i++)
            out.push_back(clampInt(center + rng.range(-spread, spread), 0, diskSize - 1));
    } else if (type == "sequential") {                // small increasing steps, wrapping
        int cur = rng.range(0, diskSize - 1);
        for (int i = 0; i < n; i++) {
            out.push_back(cur);
            cur = (cur + rng.range(1, 5)) % diskSize;
        }
    } else if (type == "skewed") {                    // 80% of requests in 20% of the disk
        int hot = diskSize / 5 > 1 ? diskSize / 5 : 1;
        for (int i = 0; i < n; i++) {
            if (hot >= diskSize || rng.range(0, 99) < 80) out.push_back(rng.range(0, hot - 1));
            else out.push_back(rng.range(hot, diskSize - 1));
        }
    } else {
        throw invalid_argument("unknown workload type '" + type + "'");
    }
    return out;
}

// ---- test suite ----
struct TestCase {
    string id, category, desc;
    int disk, head;
    vector<int> req;
    string dir;
    long long expected[5];   // FCFS, SSTF, SCAN, C-SCAN, LOOK
};

// Each request must appear exactly once in the real part of the order.
static bool servesAll(const Row& row, const vector<int>& req) {
    if (row.order.size() != req.size() + row.virt.size()) return false;
    vector<int> a = mergeSort(req), b;
    for (size_t i = 0; i < row.order.size(); i++) {
        bool v = false;
        for (size_t k = 0; k < row.virt.size(); k++) if (row.virt[k] == row.order[i]) v = true;
        if (!v) b.push_back(row.order[i]);
    }
    return mergeSort(b) == a;
}

template <typename F>
static bool throwsInvalid(F f) {
    try { f(); } catch (const invalid_argument&) { return true; }
    return false;
}

int runTests() {
    vector<int> book = {98, 183, 37, 122, 14, 124, 65, 67};
    vector<TestCase> tests = {
        // Normal
        {"N1", "Normal",   "textbook, head 53, right",          200, 53, book, "right", {640, 236, 331, 382, 299}},
        {"N2", "Normal",   "textbook, head 53, left",           200, 53, book, "left",  {640, 236, 236, 386, 208}},
        // Boundary
        {"B1", "Boundary", "single request 100, head 50",       200, 50,  {100},          "right", {50, 50, 149, 149, 50}},
        {"B2", "Boundary", "head at 0, reqs 10 20 30",          200, 0,   {10, 20, 30},   "right", {30, 30, 199, 199, 30}},
        {"B3", "Boundary", "head at max 199, reqs 10 20 30",    200, 199, {10, 20, 30},   "right", {209, 189, 189, 229, 189}},
        {"B4", "Boundary", "requests at both extremes 0,199",   200, 100, {0, 199},       "right", {299, 298, 298, 298, 298}},
        {"B5", "Boundary", "duplicates 50 50 70, head 50",      200, 50,  {50, 50, 70},   "right", {20, 20, 149, 149, 20}},
        {"B6", "Boundary", "all requests below head",           200, 100, {10, 20, 30},   "right", {110, 90, 288, 328, 90}},
        {"B7", "Boundary", "request equal to head, right",      200, 53,  {53, 98},       "right", {45, 45, 146, 146, 45}},
        {"B8", "Boundary", "request equal to head, left",       200, 53,  {53, 98},       "left",  {45, 45, 151, 353, 45}},
    };

    cout << "=== TEST CASES ===\n";
    cout << left << setw(5) << "ID" << setw(10) << "Category" << setw(38) << "Input"
         << setw(26) << "Expected F/S/SC/CS/L" << setw(26) << "Actual" << "Result\n";
    cout << string(112, '-') << "\n";

    int failed = 0;
    for (size_t t = 0; t < tests.size(); t++) {
        const TestCase& tc = tests[t];
        bool ok = true;
        ostringstream exp, act;
        for (int a = 0; a < 5; a++) {
            Row row = runAlgorithm(ALGOS[a], tc.req, tc.head, tc.disk, tc.dir);
            if (row.total != tc.expected[a]) ok = false;
            if (!servesAll(row, tc.req)) ok = false;
            exp << tc.expected[a] << (a < 4 ? "/" : "");
            act << row.total << (a < 4 ? "/" : "");
        }
        if (!ok) failed++;
        cout << left << setw(5) << tc.id << setw(10) << tc.category << setw(38) << tc.desc
             << setw(26) << exp.str() << setw(26) << act.str() << (ok ? "PASS" : "FAIL") << "\n";
    }

    // Failure tests: each must raise an error
    struct FailTest { string id, desc; bool (*check)(); };
    vector<int> okReq = {10, 20};
    FailTest fails[] = {
        {"F1", "empty request list",    [] { return throwsInvalid([] { validate(200, 50, {}, "right"); }); }},
        {"F2", "request off the disk",  [] { return throwsInvalid([] { validate(200, 50, {200}, "right"); }); }},
        {"F3", "negative request",      [] { return throwsInvalid([] { validate(200, 50, {-5}, "right"); }); }},
        {"F4", "head off the disk",     [] { return throwsInvalid([] { validate(200, 200, {5}, "right"); }); }},
        {"F5", "zero-size disk",        [] { return throwsInvalid([] { validate(0, 0, {0}, "right"); }); }},
        {"F6", "invalid direction",     [] { return throwsInvalid([] { validate(200, 50, {5}, "up"); }); }},
        {"F7", "non-integer request",   [] { return throwsInvalid([] { parseInt("3.5"); }); }},
        {"F8", "non-numeric request",   [] { return throwsInvalid([] { parseInt("abc"); }); }},
        {"F9", "algorithm rejects bad input", [] { return throwsInvalid([] { scan({300}, 50, 200, "right"); }); }},
    };
    for (size_t i = 0; i < sizeof(fails) / sizeof(fails[0]); i++) {
        bool ok = fails[i].check();
        if (!ok) failed++;
        cout << left << setw(5) << fails[i].id << setw(10) << "Failure" << setw(38) << fails[i].desc
             << setw(26) << "error raised" << setw(26) << (ok ? "error raised" : "no error")
             << (ok ? "PASS" : "FAIL") << "\n";
    }

    // Data-structure unit tests
    bool u1, u2, u3, u4;
    {
        LinkedList ll;
        ll.append(1); ll.append(2); ll.append(3);
        u1 = (ll.toVector() == vector<int>{1, 2, 3} && ll.tail()->value == 3 && ll.size() == 3);
        ll.removeNode(ll.head()->next, ll.tail());            // remove tail
        u2 = (ll.toVector() == vector<int>{1, 2} && ll.tail()->value == 2);
        ll.removeNode(nullptr, ll.head());                    // remove head
        ll.removeNode(nullptr, ll.head());                    // remove last node -> empty
        ll.append(9);                                         // still works after emptying
        u2 = u2 && ll.toVector() == vector<int>{9} && ll.size() == 1;
    }
    u3 = (mergeSort({5, 3, 5, 1, 0, 9}) == vector<int>{0, 1, 3, 5, 5, 9}) &&
         mergeSort({}).empty() && (mergeSort({7}) == vector<int>{7});
    {
        vector<int> big = generateWorkload("random", 500, 1000, 7);
        vector<int> s = mergeSort(big);
        u4 = true;
        for (size_t i = 1; i < s.size(); i++) if (s[i - 1] > s[i]) u4 = false;
        u4 = u4 && s.size() == big.size();
    }
    struct U { const char* id; const char* desc; bool ok; };
    U units[] = {{"U1", "linked list append/tail/size", u1}, {"U2", "linked list remove/reuse", u2},
                 {"U3", "merge sort small cases", u3},     {"U4", "merge sort 500 random values", u4}};
    for (int i = 0; i < 4; i++) {
        if (!units[i].ok) failed++;
        cout << left << setw(5) << units[i].id << setw(10) << "Unit" << setw(38) << units[i].desc
             << setw(26) << "correct" << setw(26) << (units[i].ok ? "correct" : "wrong")
             << (units[i].ok ? "PASS" : "FAIL") << "\n";
    }

    cout << "\n" << (failed == 0 ? "ALL TESTS PASSED" : to_string(failed) + " TEST(S) FAILED") << "\n";
    return failed == 0 ? 0 : 1;
}

// ---- experiments ----
void runExperiments(bool big) {
    const int DISK = 5000;
    const int HEAD = DISK / 2;
    const int SEEDS = 5;
    vector<int> sizes = {10, 100, 1000};
    if (big) sizes.push_back(10000);
    const char* types[4] = {"random", "clustered", "sequential", "skewed"};

    ofstream csv("results.csv");
    csv << "workload,size,algorithm,avg_total_movement,avg_seek\n";
    cout << "Experiments: disk 0-" << DISK - 1 << ", head " << HEAD << ", direction right, average of "
         << SEEDS << " seeds. Every algorithm sees the identical workload.\n\n";

    for (int w = 0; w < 4; w++) {
        for (size_t s = 0; s < sizes.size(); s++) {
            int n = sizes[s];
            double sum[5] = {0, 0, 0, 0, 0};
            for (int seed = 1; seed <= SEEDS; seed++) {
                vector<int> req = generateWorkload(types[w], n, DISK, (uint64_t)seed);
                for (int a = 0; a < 5; a++)
                    sum[a] += (double)runAlgorithm(ALGOS[a], req, HEAD, DISK, "right").total;
            }
            cout << types[w] << ", n=" << n << "\n";
            int bestIdx = 0;
            for (int a = 0; a < 5; a++) if (sum[a] < sum[bestIdx]) bestIdx = a;
            for (int a = 0; a < 5; a++) {
                double avgTotal = sum[a] / SEEDS;
                double avgSeek = avgTotal / n;
                cout << "   " << left << setw(8) << ALGOS[a].name << right << setw(12) << fixed
                     << setprecision(1) << avgTotal << "  avg seek " << setprecision(2) << avgSeek
                     << (a == bestIdx ? "   <- best" : "") << "\n";
                csv << types[w] << "," << n << "," << ALGOS[a].name << "," << fixed
                    << setprecision(1) << avgTotal << "," << setprecision(3) << avgSeek << "\n";
            }
            cout << "\n";
        }
    }
    cout << "Saved results.csv (use plot.py for the charts).\n";
}

// ============================================================================
// Demo and interactive run
// ============================================================================
void demo() {
    vector<int> book = {98, 183, 37, 122, 14, 124, 65, 67};
    vector<Row> rows = compare(book, 53, 200, "right", true);
    saveAllPaths(rows, 53);
    cout << "\nExpected: FCFS 640, SSTF 236, SCAN 331, C-SCAN 382, LOOK 299\n";
    cout << "Head paths saved as path_<ALGO>.csv (plot with plot.py).\n";
}

void interactiveRun() {
    try {
        string line;
        cout << "Disk size: ";           getline(cin, line); int disk = parseInt(line);
        cout << "Initial head: ";        getline(cin, line); int head = parseInt(line);
        cout << "Direction (left/right): "; string dir; getline(cin, dir);
        cout << "Requests separated by spaces, or: random <type> <count> <seed>\n"
             << "(types: random clustered sequential skewed)\n> ";
        getline(cin, line);
        istringstream in(line);
        vector<string> tok;
        string t;
        while (in >> t) tok.push_back(t);
        vector<int> req;
        if (!tok.empty() && tok[0] == "random") {
            if (tok.size() != 4) throw invalid_argument("usage: random <type> <count> <seed>");
            req = generateWorkload(tok[1], parseInt(tok[2]), disk, (uint64_t)parseInt(tok[3]));
        } else {
            for (size_t i = 0; i < tok.size(); i++) req.push_back(parseInt(tok[i]));
        }
        validate(disk, head, req, dir);
        vector<Row> rows = compare(req, head, disk, dir, true);
        saveAllPaths(rows, head);
        cout << "Head paths saved as path_<ALGO>.csv\n";
    } catch (const exception& e) {
        cout << "Error: " << e.what() << "\n";
    }
}

int main(int argc, char** argv) {
    string mode = argc > 1 ? argv[1] : "demo";
    if (mode == "demo") demo();
    else if (mode == "test") return runTests();
    else if (mode == "experiment") runExperiments(argc > 2 && string(argv[2]) == "--big");
    else if (mode == "run") interactiveRun();
    else cout << "Usage: disk_scheduler [demo|test|experiment [--big]|run]\n";
    return 0;
}