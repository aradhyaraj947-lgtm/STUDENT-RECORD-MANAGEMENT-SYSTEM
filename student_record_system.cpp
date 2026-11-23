/*
 * Student Record Management System  (C++ / Data Structures)
 * ---------------------------------------------------------
 * Data structures used:
 *   1. AVL Tree        -> primary index by Roll No (insert/search/delete in O(log n))
 *   2. Hash Table      -> secondary index by Name (separate chaining, O(1) avg)
 *   3. Stack           -> Undo for add / delete / update
 *   4. Queue           -> Pending admission requests (FIFO)
 *   5. Merge Sort      -> Sort records by CGPA / Name / Roll
 *   6. File I/O (CSV)  -> Persistence
 *
 * Compile:  g++ -std=c++17 -O2 student_record_system.cpp -o srms
 * Run:      ./srms
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <functional>
using namespace std;

// ============================================================
//  Student record
// ============================================================
struct Student {
    int    roll     = 0;
    string name;
    string course;
    int    semester = 0;
    float  cgpa     = 0.0f;
};

// ============================================================
//  1. AVL TREE  (key = roll number)
// ============================================================
struct Node {
    Student data;
    Node *left, *right;
    int height;
    explicit Node(const Student& s) : data(s), left(nullptr), right(nullptr), height(1) {}
};

class AVLTree {
    Node* root = nullptr;
    int count  = 0;

    static int h(Node* n) { return n ? n->height : 0; }
    static void update(Node* n) { n->height = 1 + max(h(n->left), h(n->right)); }
    static int balance(Node* n) { return n ? h(n->left) - h(n->right) : 0; }

    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        x->right = y;
        update(y); update(x);
        return x;
    }
    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        y->left = x;
        update(x); update(y);
        return y;
    }
    static Node* rebalance(Node* n) {
        update(n);
        int b = balance(n);
        if (b > 1) {                                   // left heavy
            if (balance(n->left) < 0) n->left = rotateLeft(n->left);   // LR
            return rotateRight(n);                                      // LL
        }
        if (b < -1) {                                  // right heavy
            if (balance(n->right) > 0) n->right = rotateRight(n->right); // RL
            return rotateLeft(n);                                        // RR
        }
        return n;
    }

    Node* insert(Node* n, const Student& s, bool& ok) {
        if (!n) { ok = true; return new Node(s); }
        if (s.roll < n->data.roll)      n->left  = insert(n->left, s, ok);
        else if (s.roll > n->data.roll) n->right = insert(n->right, s, ok);
        else { ok = false; return n; }                 // duplicate roll
        return rebalance(n);
    }

    static Node* minNode(Node* n) { while (n->left) n = n->left; return n; }

    Node* remove(Node* n, int roll, bool& ok) {
        if (!n) return nullptr;
        if (roll < n->data.roll)      n->left  = remove(n->left, roll, ok);
        else if (roll > n->data.roll) n->right = remove(n->right, roll, ok);
        else {
            ok = true;
            if (!n->left || !n->right) {               // 0 or 1 child
                Node* child = n->left ? n->left : n->right;
                delete n;
                return child;
            }
            Node* succ = minNode(n->right);            // 2 children: inorder successor
            n->data = succ->data;
            n->right = remove(n->right, succ->data.roll, ok);
        }
        return rebalance(n);
    }

    void inorder(Node* n, vector<Student>& out) const {
        if (!n) return;
        inorder(n->left, out);
        out.push_back(n->data);
        inorder(n->right, out);
    }

    void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

public:
    ~AVLTree() { destroy(root); }

    bool insert(const Student& s) {
        bool ok = false;
        root = insert(root, s, ok);
        if (ok) count++;
        return ok;
    }

    bool remove(int roll) {
        bool ok = false;
        root = remove(root, roll, ok);
        if (ok) count--;
        return ok;
    }

    Student* find(int roll) {
        Node* cur = root;
        while (cur) {
            if (roll == cur->data.roll) return &cur->data;
            cur = (roll < cur->data.roll) ? cur->left : cur->right;
        }
        return nullptr;
    }

    vector<Student> toVector() const {
        vector<Student> v;
        v.reserve(count);
        inorder(root, v);
        return v;
    }

    int  size() const { return count; }
    int  treeHeight() const { return h(root); }
    void clear() { destroy(root); root = nullptr; count = 0; }
};

// ============================================================
//  2. HASH TABLE  (key = lowercase name, separate chaining)
//     Maps name -> list of roll numbers (names can repeat)
// ============================================================
class NameHashTable {
    static const int SIZE = 101;
    vector<list<pair<string, int>>> table;

    static string lower(string s) {
        for (auto& c : s) c = (char)tolower((unsigned char)c);
        return s;
    }
    static size_t hashFn(const string& k) {
        size_t hv = 5381;                              // djb2
        for (char c : k) hv = hv * 33 + (unsigned char)c;
        return hv % SIZE;
    }

public:
    NameHashTable() : table(SIZE) {}

    void insert(const string& name, int roll) {
        string k = lower(name);
        table[hashFn(k)].push_back({k, roll});
    }

    void erase(const string& name, int roll) {
        string k = lower(name);
        auto& bucket = table[hashFn(k)];
        bucket.remove_if([&](const pair<string, int>& p) {
            return p.first == k && p.second == roll;
        });
    }

    vector<int> find(const string& name) const {
        string k = lower(name);
        vector<int> rolls;
        for (const auto& p : table[hashFn(k)])
            if (p.first == k) rolls.push_back(p.second);
        return rolls;
    }

    void clear() { for (auto& b : table) b.clear(); }
};

// ============================================================
//  5. MERGE SORT  (generic, with comparator)
// ============================================================
using Comparator = function<bool(const Student&, const Student&)>;

void mergeSort(vector<Student>& a, int l, int r, const Comparator& less) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSort(a, l, m, less);
    mergeSort(a, m + 1, r, less);

    vector<Student> tmp;
    tmp.reserve(r - l + 1);
    int i = l, j = m + 1;
    while (i <= m && j <= r)
        tmp.push_back(less(a[j], a[i]) ? a[j++] : a[i++]);   // stable
    while (i <= m) tmp.push_back(a[i++]);
    while (j <= r) tmp.push_back(a[j++]);
    for (int k = 0; k < (int)tmp.size(); k++) a[l + k] = tmp[k];
}

// ============================================================
//  3. UNDO STACK action
// ============================================================
enum class ActionType { ADD, REMOVE, UPDATE };

struct Action {
    ActionType type;
    Student before;   // state before the action (REMOVE / UPDATE)
    Student after;    // state after the action  (ADD / UPDATE)
};

// ============================================================
//  Manager: ties all structures together (the "backend")
// ============================================================
class StudentManager {
    AVLTree        tree;
    NameHashTable  nameIndex;
    stack<Action>  undoStack;
    queue<Student> admissionQueue;       // 4. Queue
    const string   FILE_NAME = "students.csv";

    static string clean(string s) {      // commas would break CSV
        replace(s.begin(), s.end(), ',', ' ');
        return s;
    }

    static void printHeader() {
        cout << "\n" << left
             << setw(8)  << "Roll"
             << setw(22) << "Name"
             << setw(16) << "Course"
             << setw(6)  << "Sem"
             << setw(6)  << "CGPA" << "\n"
             << string(58, '-') << "\n";
    }
    static void printRow(const Student& s) {
        cout << left
             << setw(8)  << s.roll
             << setw(22) << s.name
             << setw(16) << s.course
             << setw(6)  << s.semester
             << fixed << setprecision(2) << s.cgpa << "\n";
    }

    // internal add without undo bookkeeping
    bool rawAdd(const Student& s) {
        if (!tree.insert(s)) return false;
        nameIndex.insert(s.name, s.roll);
        return true;
    }
    bool rawRemove(int roll) {
        Student* p = tree.find(roll);
        if (!p) return false;
        nameIndex.erase(p->name, roll);
        return tree.remove(roll);
    }

public:
    // ---------- validation ----------
    static bool valid(const Student& s, string& err) {
        if (s.roll <= 0)                       { err = "Roll number must be positive."; return false; }
        if (s.name.empty())                    { err = "Name cannot be empty.";         return false; }
        if (s.semester < 1 || s.semester > 8)  { err = "Semester must be 1-8.";         return false; }
        if (s.cgpa < 0.0f || s.cgpa > 10.0f)   { err = "CGPA must be 0-10.";            return false; }
        return true;
    }

    // ---------- CRUD ----------
    bool addStudent(Student s) {
        s.name = clean(s.name);
        s.course = clean(s.course);
        string err;
        if (!valid(s, err)) { cout << "  ! " << err << "\n"; return false; }
        if (!rawAdd(s)) { cout << "  ! Roll number " << s.roll << " already exists.\n"; return false; }
        undoStack.push({ActionType::ADD, Student(), s});
        cout << "  + Student added.\n";
        return true;
    }

    bool deleteStudent(int roll) {
        Student* p = tree.find(roll);
        if (!p) { cout << "  ! No student with roll " << roll << ".\n"; return false; }
        Student old = *p;
        rawRemove(roll);
        undoStack.push({ActionType::REMOVE, old, Student()});
        cout << "  - Student deleted.\n";
        return true;
    }

    bool updateStudent(int roll, Student updated) {
        Student* p = tree.find(roll);
        if (!p) { cout << "  ! No student with roll " << roll << ".\n"; return false; }
        updated.roll = roll;                       // roll is the key; cannot change
        updated.name = clean(updated.name);
        updated.course = clean(updated.course);
        string err;
        if (!valid(updated, err)) { cout << "  ! " << err << "\n"; return false; }

        Student old = *p;
        nameIndex.erase(old.name, roll);
        *p = updated;
        nameIndex.insert(updated.name, roll);
        undoStack.push({ActionType::UPDATE, old, updated});
        cout << "  ~ Student updated.\n";
        return true;
    }

    // ---------- Search ----------
    void searchByRoll(int roll) {
        Student* p = tree.find(roll);
        if (!p) { cout << "  ! Not found.\n"; return; }
        printHeader(); printRow(*p);
    }

    void searchByName(const string& name) {
        vector<int> rolls = nameIndex.find(name);
        if (rolls.empty()) { cout << "  ! No student named \"" << name << "\".\n"; return; }
        printHeader();
        for (int r : rolls) if (Student* p = tree.find(r)) printRow(*p);
    }

    void searchByPartialName(const string& part) {      // linear scan over inorder list
        string q = part;
        transform(q.begin(), q.end(), q.begin(), [](unsigned char c){ return tolower(c); });
        bool any = false;
        for (const auto& s : tree.toVector()) {
            string n = s.name;
            transform(n.begin(), n.end(), n.begin(), [](unsigned char c){ return tolower(c); });
            if (n.find(q) != string::npos) {
                if (!any) { printHeader(); any = true; }
                printRow(s);
            }
        }
        if (!any) cout << "  ! No matches.\n";
    }

    // ---------- Display / Sort ----------
    void displayAll() {
        auto v = tree.toVector();                       // sorted by roll (inorder)
        if (v.empty()) { cout << "  (no records)\n"; return; }
        printHeader();
        for (const auto& s : v) printRow(s);
        cout << "\nTotal: " << v.size() << " students\n";
    }

    void displaySorted(int mode) {                      // 1=CGPA desc, 2=Name, 3=Course
        auto v = tree.toVector();
        if (v.empty()) { cout << "  (no records)\n"; return; }
        Comparator cmp;
        if (mode == 1)      cmp = [](const Student& a, const Student& b){ return a.cgpa > b.cgpa; };
        else if (mode == 2) cmp = [](const Student& a, const Student& b){ return a.name < b.name; };
        else                cmp = [](const Student& a, const Student& b){ return a.course < b.course; };
        mergeSort(v, 0, (int)v.size() - 1, cmp);
        printHeader();
        for (const auto& s : v) printRow(s);
    }

    void topN(int n) {
        auto v = tree.toVector();
        if (v.empty()) { cout << "  (no records)\n"; return; }
        mergeSort(v, 0, (int)v.size() - 1,
                  [](const Student& a, const Student& b){ return a.cgpa > b.cgpa; });
        printHeader();
        for (int i = 0; i < n && i < (int)v.size(); i++) printRow(v[i]);
    }

    void statistics() {
        auto v = tree.toVector();
        if (v.empty()) { cout << "  (no records)\n"; return; }
        double sum = 0;
        const Student *best = &v[0], *worst = &v[0];
        for (const auto& s : v) {
            sum += s.cgpa;
            if (s.cgpa > best->cgpa)  best = &s;
            if (s.cgpa < worst->cgpa) worst = &s;
        }
        cout << fixed << setprecision(2)
             << "\n  Total students : " << v.size()
             << "\n  Average CGPA   : " << sum / v.size()
             << "\n  Highest CGPA   : " << best->cgpa  << " (" << best->name  << ", Roll " << best->roll  << ")"
             << "\n  Lowest CGPA    : " << worst->cgpa << " (" << worst->name << ", Roll " << worst->roll << ")"
             << "\n  AVL tree height: " << tree.treeHeight() << "\n";
    }

    // ---------- Undo (stack) ----------
    void undo() {
        if (undoStack.empty()) { cout << "  ! Nothing to undo.\n"; return; }
        Action a = undoStack.top(); undoStack.pop();
        switch (a.type) {
            case ActionType::ADD:
                rawRemove(a.after.roll);
                cout << "  <- Undid ADD of roll " << a.after.roll << ".\n";
                break;
            case ActionType::REMOVE:
                rawAdd(a.before);
                cout << "  <- Undid DELETE of roll " << a.before.roll << ".\n";
                break;
            case ActionType::UPDATE: {
                Student* p = tree.find(a.before.roll);
                if (p) {
                    nameIndex.erase(p->name, p->roll);
                    *p = a.before;
                    nameIndex.insert(p->name, p->roll);
                }
                cout << "  <- Undid UPDATE of roll " << a.before.roll << ".\n";
                break;
            }
        }
    }

    // ---------- Admission queue (FIFO) ----------
    void requestAdmission(Student s) {
        s.name = clean(s.name);
        s.course = clean(s.course);
        string err;
        if (!valid(s, err)) { cout << "  ! " << err << "\n"; return; }
        admissionQueue.push(s);
        cout << "  Request queued. Pending: " << admissionQueue.size() << "\n";
    }

    void processNextAdmission() {
        if (admissionQueue.empty()) { cout << "  ! No pending requests.\n"; return; }
        Student s = admissionQueue.front(); admissionQueue.pop();
        cout << "  Processing request for " << s.name << " (Roll " << s.roll << ")...\n";
        addStudent(s);
    }

    void showPending() {
        if (admissionQueue.empty()) { cout << "  (queue empty)\n"; return; }
        queue<Student> copy = admissionQueue;
        printHeader();
        while (!copy.empty()) { printRow(copy.front()); copy.pop(); }
    }

    // ---------- File persistence ----------
    bool saveToFile() {
        ofstream out(FILE_NAME);
        if (!out) { cout << "  ! Cannot write " << FILE_NAME << ".\n"; return false; }
        for (const auto& s : tree.toVector())
            out << s.roll << ',' << s.name << ',' << s.course << ','
                << s.semester << ',' << s.cgpa << '\n';
        cout << "  Saved " << tree.size() << " records to " << FILE_NAME << ".\n";
        return true;
    }

    bool loadFromFile() {
        ifstream in(FILE_NAME);
        if (!in) return false;                         // first run: no file yet
        tree.clear(); nameIndex.clear();
        string line; int loaded = 0;
        while (getline(in, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string roll, name, course, sem, cgpa;
            getline(ss, roll, ',');  getline(ss, name, ',');
            getline(ss, course, ','); getline(ss, sem, ','); getline(ss, cgpa, ',');
            try {
                Student s;
                s.roll = stoi(roll); s.name = name; s.course = course;
                s.semester = stoi(sem); s.cgpa = stof(cgpa);
                if (rawAdd(s)) loaded++;
            } catch (...) { /* skip malformed line */ }
        }
        cout << "  Loaded " << loaded << " records from " << FILE_NAME << ".\n";
        return true;
    }
};

// ============================================================
//  Input helpers
// ============================================================
int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) exit(0);              // EOF
        try {
            size_t pos;
            int v = stoi(line, &pos);
            if (pos == line.size()) return v;
        } catch (...) {}
        cout << "  ! Enter a valid integer.\n";
    }
}

float readFloat(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) exit(0);
        try {
            size_t pos;
            float v = stof(line, &pos);
            if (pos == line.size()) return v;
        } catch (...) {}
        cout << "  ! Enter a valid number.\n";
    }
}

string readLine(const string& prompt) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) exit(0);
    return line;
}

Student readStudent(int roll = -1) {
    Student s;
    s.roll     = (roll > 0) ? roll : readInt("  Roll No   : ");
    s.name     = readLine("  Name      : ");
    s.course   = readLine("  Course    : ");
    s.semester = readInt ("  Semester  : ");
    s.cgpa     = readFloat("  CGPA      : ");
    return s;
}

// ============================================================
//  Menu
// ============================================================
void printMenu() {
    cout << "\n========= STUDENT RECORD MANAGEMENT SYSTEM =========\n"
         << " 1. Add student              9. Statistics\n"
         << " 2. Delete student          10. Undo last action\n"
         << " 3. Update student          11. Queue admission request\n"
         << " 4. Search by roll no       12. Process next admission\n"
         << " 5. Search by name (exact)  13. Show pending admissions\n"
         << " 6. Search by name (partial)14. Save to file\n"
         << " 7. Display all            15. Load from file\n"
         << " 8. Sorted view / Top N      0. Exit\n"
         << "=====================================================\n";
}

int main() {
    StudentManager mgr;
    mgr.loadFromFile();                                // auto-load on start

    while (true) {
        printMenu();
        int choice = readInt("Choice: ");
        switch (choice) {
            case 1:  mgr.addStudent(readStudent()); break;
            case 2:  mgr.deleteStudent(readInt("  Roll No: ")); break;
            case 3: {
                int r = readInt("  Roll No to update: ");
                cout << "  Enter new details:\n";
                mgr.updateStudent(r, readStudent(r));
                break;
            }
            case 4:  mgr.searchByRoll(readInt("  Roll No: ")); break;
            case 5:  mgr.searchByName(readLine("  Name: ")); break;
            case 6:  mgr.searchByPartialName(readLine("  Name contains: ")); break;
            case 7:  mgr.displayAll(); break;
            case 8: {
                int m = readInt("  1) CGPA desc  2) Name  3) Course  4) Top N : ");
                if (m == 4) mgr.topN(readInt("  N: "));
                else        mgr.displaySorted(m);
                break;
            }
            case 9:  mgr.statistics(); break;
            case 10: mgr.undo(); break;
            case 11: mgr.requestAdmission(readStudent()); break;
            case 12: mgr.processNextAdmission(); break;
            case 13: mgr.showPending(); break;
            case 14: mgr.saveToFile(); break;
            case 15: mgr.loadFromFile(); break;
            case 0:
                mgr.saveToFile();
                cout << "Goodbye!\n";
                return 0;
            default: cout << "  ! Invalid choice.\n";
        }
    }
}
