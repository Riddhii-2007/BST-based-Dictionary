/*
 * ============================================================
 *  BST-Based Dictionary  —  Vardhaman College of Engineering
 *  CSE Department  |  Summer Project 2025-26
 *
 *  CONTRIBUTOR : Araveti Harika
 *  ROLE        : BST Core Operations — Search, Insertion, Deletion
 *  COMPLEXITY  : O(log n) average for all three operations
 * ============================================================
 *
 *  This file contains:
 *    [1] WordEntry struct         — Data model for each word
 *    [2] Node struct              — BST tree node
 *    [3] BST class (Rule of 5)   — Full BST implementation
 *         - insert()             — Recursive insertion, O(log n) avg
 *         - search()             — Returns optional<ref>, O(log n) avg
 *         - remove()             — In-order successor delete, O(log n)
 *         - findMin()            — Leftmost node helper
 *         - deleteNode()         — Recursive delete (3 cases handled)
 *         - cloneSubtree()       — Deep copy for Rule-of-5
 *
 *  COMPILE : g++ -std=c++17 -Wall -o harika_bst harika_bst_core.cpp
 *  RUN     : ./harika_bst
 * ============================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>
#include <cctype>
#include <climits>

using std::string;
using std::vector;
using std::optional;
using std::cout;

// ============================================================
//  ANSI COLOR CODES
// ============================================================
namespace Color {
    const string RESET   = "\033[0m";
    const string BOLD    = "\033[1m";
    const string GREEN   = "\033[32m";
    const string YELLOW  = "\033[33m";
    const string RED     = "\033[31m";
    const string CYAN    = "\033[36m";
    const string MAGENTA = "\033[35m";
    const string DIM     = "\033[2m";
    const string GRAY    = "\033[90m";
}

// ============================================================
//  UTILITY — toLower (needed for key normalisation)
// ============================================================
string toLower(const string& s) {
    string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return r;
}

// ============================================================
//  DATA STRUCTURES
// ============================================================

/**
 * @struct WordEntry
 * @brief  Complete data for a single dictionary entry.
 *
 * @field word        Primary BST key; always stored lowercase.
 * @field definition  Human-readable meaning (required, non-empty).
 * @field wordType    Grammatical category: Noun, Verb, Adjective, etc.
 * @field phonetic    IPA / simplified phonetic string (optional).
 * @field synonyms    Related words, up to 10 (optional).
 *
 * Owner: Araveti Harika — this struct is the atom of every BST node.
 */
struct WordEntry {
    string         word;
    string         definition;
    string         wordType;
    string         phonetic;
    vector<string> synonyms;
};

/**
 * @struct Node
 * @brief  Single BST node wrapping a WordEntry.
 *
 * Ordering: lexicographic on WordEntry::word (lowercase).
 *   left  → alphabetically BEFORE this node
 *   right → alphabetically AFTER  this node
 *
 * Memory ownership: each Node is heap-allocated; the BST class
 * manages lifetime via freeTree() called from the destructor.
 */
struct Node {
    WordEntry data;
    Node*     left  = nullptr;
    Node*     right = nullptr;

    /** Constructs a leaf node — both children are null. */
    explicit Node(const WordEntry& e) : data(e) {}
};

// ============================================================
//  BST CLASS  —  Primary Contribution by Araveti Harika
// ============================================================

/**
 * @class BST
 * @brief Unbalanced Binary Search Tree for dictionary storage.
 *
 * ── Why BST? ────────────────────────────────────────────────
 * A BST keeps words in sorted order (alphabetical) without any
 * extra sort pass.  In-order traversal gives A→Z for free.
 * Average insert/search/delete are all O(log n).
 *
 * ── Rule of 5  [FIX-02] ────────────────────────────────────
 * The BST owns heap Nodes, so C++'s "Rule of 5" applies.
 * If we let the compiler generate copy/move, two BSTs would
 * share the same raw Node* → double-free on destruction.
 * We therefore define all five:
 *   1. Destructor          (freeTree — post-order delete)
 *   2. Copy constructor    (cloneSubtree — deep copy)
 *   3. Copy assignment     (copy-and-swap idiom)
 *   4. Move constructor    (steal root, null other)
 *   5. Move assignment     (swap + let other destruct old)
 *
 * ── Safe search return  [FIX-01] ───────────────────────────
 * search() returns optional<reference_wrapper<WordEntry>>.
 * Callers can never accidentally delete or dangle the pointer;
 * they get a reference they can read/write, not own.
 */
class BST {
public:
    using OptRef = optional<std::reference_wrapper<WordEntry>>;

private:
    Node* root = nullptr;

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Deep-copy an entire subtree
    //  Used by copy constructor and copy-assignment operator.
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Recursively clones a subtree (pre-order traversal).
     * @param  src  Root of subtree to clone (nullptr is safe).
     * @return      Root of the new, independent copy.
     * @complexity  O(n) time and O(n) space.
     */
    static Node* cloneSubtree(const Node* src) {
        if (!src) return nullptr;
        Node* n   = new Node(src->data);    // copy WordEntry value
        n->left   = cloneSubtree(src->left);
        n->right  = cloneSubtree(src->right);
        return n;
    }

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Recursive insertion
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Inserts e into the subtree rooted at node.
     *
     * Traversal:
     *   e.word < node.word → go LEFT  (smaller words live left)
     *   e.word > node.word → go RIGHT (larger  words live right)
     *   e.word == node.word→ duplicate: silently reject
     *
     * @param  node      Current subtree root (nullptr = empty slot).
     * @param  e         Entry to insert.
     * @param  inserted  Set true iff a new node was allocated.
     * @return           Updated subtree root.
     * @complexity       O(log n) average; O(n) worst (degenerate tree).
     */
    Node* insert(Node* node, const WordEntry& e, bool& inserted) {
        if (!node) {
            inserted = true;
            return new Node(e);             // base case: empty slot found
        }
        if (e.word < node->data.word)
            node->left  = insert(node->left,  e, inserted);
        else if (e.word > node->data.word)
            node->right = insert(node->right, e, inserted);
        else
            inserted = false;               // duplicate key — reject
        return node;
    }

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Recursive search
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Locates the node whose word matches key.
     *
     * At each step:
     *   key == node.word → found, return node
     *   key <  node.word → search LEFT subtree
     *   key >  node.word → search RIGHT subtree
     *
     * @param  node  Current subtree root.
     * @param  key   Lowercase target word.
     * @return       Pointer to the matching node, or nullptr.
     * @complexity   O(log n) average.
     */
    Node* search(Node* node, const string& key) const {
        if (!node || node->data.word == key) return node;
        return (key < node->data.word)
               ? search(node->left,  key)
               : search(node->right, key);
    }

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Find minimum (leftmost node)
    //  Used by deleteNode() to locate the in-order successor.
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Returns the leftmost node in a non-null subtree.
     *         (= smallest key in the subtree)
     * @param  node  Root of non-empty subtree.
     * @return       Pointer to leftmost node.
     * @complexity   O(h) where h = subtree height.
     */
    Node* findMin(Node* node) const {
        while (node->left) node = node->left;
        return node;
    }

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Recursive delete
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Removes the node with the given key from the subtree.
     *
     * Three structural cases are handled:
     *
     *   Case 1 — Leaf node (no children):
     *     Free the node directly; return nullptr.
     *
     *   Case 2 — One child (left or right):
     *     Link the parent past the deleted node to its only child.
     *     Free the deleted node.
     *
     *   Case 3 — Two children:
     *     Find the in-order successor (smallest node in right subtree).
     *     Copy successor's data into current node.
     *     Recursively delete the successor from the right subtree.
     *     (The successor has at most one right child, so this reduces
     *      to Case 1 or Case 2.)
     *
     * @param  node  Current subtree root.
     * @param  key   Word to delete.
     * @return       Updated subtree root.
     * @complexity   O(log n) average.
     */
    Node* deleteNode(Node* node, const string& key) {
        if (!node) return nullptr;               // key not found — no-op

        if (key < node->data.word) {
            node->left  = deleteNode(node->left,  key);
        } else if (key > node->data.word) {
            node->right = deleteNode(node->right, key);
        } else {
            // ── CASE 1: Leaf ─────────────────────────────────
            if (!node->left && !node->right) {
                delete node;
                return nullptr;
            }
            // ── CASE 2a: Only right child ────────────────────
            if (!node->left) {
                Node* temp = node->right;
                delete node;
                return temp;
            }
            // ── CASE 2b: Only left child ─────────────────────
            if (!node->right) {
                Node* temp = node->left;
                delete node;
                return temp;
            }
            // ── CASE 3: Two children ─────────────────────────
            // In-order successor: smallest node in right subtree
            Node* succ  = findMin(node->right);
            node->data  = succ->data;            // overwrite with successor
            node->right = deleteNode(node->right, succ->data.word);
        }
        return node;
    }

    // ──────────────────────────────────────────────────────────
    //  PRIVATE HELPER: Post-order deallocation
    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Frees every node in post-order (children before parent).
     * @param  node  Subtree root.
     * @complexity   O(n).
     */
    void freeTree(Node* node) {
        if (!node) return;
        freeTree(node->left);
        freeTree(node->right);
        delete node;
    }

public:
    // ══════════════════════════════════════════════════════════
    //  RULE OF 5  [FIX-02]
    // ══════════════════════════════════════════════════════════

    /** Default constructor — creates an empty tree. */
    BST() = default;

    /**
     * @brief  Copy constructor — performs a deep clone of the tree.
     * @param  other  Source BST (unchanged).
     * @complexity    O(n).
     */
    BST(const BST& other) : root(cloneSubtree(other.root)) {}

    /**
     * @brief  Copy-assignment using the copy-and-swap idiom.
     *
     * Trick: 'other' is passed BY VALUE, so the copy is already
     * made on entry.  We then steal its root and let the original
     * root be freed by other's destructor at end of scope.
     *
     * @param  other  Source BST (copy).
     * @return        *this.
     */
    BST& operator=(BST other) {
        std::swap(root, other.root);
        return *this;
    }

    /**
     * @brief  Move constructor — steals the other tree's root in O(1).
     *         Leaves 'other' in a valid, empty state.
     * @param  other  Source BST (will be empty after this call).
     */
    BST(BST&& other) noexcept : root(other.root) {
        other.root = nullptr;
    }

    /**
     * @brief  Move-assignment — swap roots then let other's destructor
     *         clean up the old root.
     * @param  other  Source BST.
     * @return        *this.
     */
    BST& operator=(BST&& other) noexcept {
        if (this != &other) std::swap(root, other.root);
        return *this;
    }

    /** Destructor — frees every node via post-order traversal. */
    ~BST() { freeTree(root); }

    // ══════════════════════════════════════════════════════════
    //  PUBLIC QUERY HELPERS
    // ══════════════════════════════════════════════════════════

    /** @return true if no entries exist. */
    bool isEmpty() const { return root == nullptr; }

    // ══════════════════════════════════════════════════════════
    //  PUBLIC API — HARIKA'S THREE CORE OPERATIONS
    // ══════════════════════════════════════════════════════════

    /**
     * @brief  Inserts a new entry into the BST.
     *
     * The word field of e must already be lowercase before calling.
     * Duplicate keys are silently rejected (returns false).
     *
     * @param  e  Entry to insert.
     * @return    true  if inserted successfully.
     *            false if key already exists (duplicate rejected).
     * @complexity O(log n) average; O(n) worst case (skewed tree).
     */
    bool insert(const WordEntry& e) {
        bool inserted = false;
        root = insert(root, e, inserted);
        return inserted;
    }

    /**
     * @brief  Searches for a word and returns a safe optional reference.
     *         [FIX-01] Returns optional<reference_wrapper<WordEntry>>
     *         instead of a raw WordEntry* to prevent dangling pointers.
     *
     * Usage:
     *   auto opt = bst.search("apple");
     *   if (opt) { WordEntry& e = opt->get(); ... }
     *
     * @param  key  Word to find (case-insensitive; normalised internally).
     * @return      optional containing a live reference to the entry,
     *              or std::nullopt if not found.
     * @complexity  O(log n) average.
     */
    OptRef search(const string& key) {
        Node* n = search(root, toLower(key));
        if (!n) return std::nullopt;
        return std::ref(n->data);
    }

    /**
     * @brief  Removes the entry for the given key.
     *         Uses the in-order successor strategy (deleteNode).
     *         No-op if key is absent.
     *
     * @param  key  Word to remove (case-insensitive).
     * @return      true  if found and deleted.
     *              false if key was not present.
     * @complexity  O(log n) average.
     */
    bool remove(const string& key) {
        const string k = toLower(key);
        if (!search(root, k)) return false;    // not found — nothing to do
        root = deleteNode(root, k);
        return true;
    }

    // ══════════════════════════════════════════════════════════
    //  ADDITIONAL ACCESSORS (used by other modules)
    // ══════════════════════════════════════════════════════════
    Node* getRoot() const { return root; }
};

// ============================================================
//  DEMONSTRATION MAIN
//  Shows insert, search (found / not found), and delete.
// ============================================================
int main() {
    BST bst;

    // ── Insert ───────────────────────────────────────────────
    cout << Color::CYAN << Color::BOLD
         << "\n  ══ HARIKA: BST Insert / Search / Delete Demo ══\n\n"
         << Color::RESET;

    auto addEntry = [&](const string& w, const string& def, const string& type) {
        WordEntry e;
        e.word       = toLower(w);
        e.definition = def;
        e.wordType   = type;
        bool ok = bst.insert(e);
        cout << Color::GREEN << "  [INSERT] " << Color::RESET
             << "'" << w << "'  → "
             << (ok ? Color::GREEN + "SUCCESS" : Color::YELLOW + "DUPLICATE (rejected)")
             << Color::RESET << "\n";
    };

    addEntry("Apple",    "A round fruit, red or green",       "Noun");
    addEntry("Binary",   "Relating to a system with base 2",  "Adjective");
    addEntry("Compile",  "To translate source code",          "Verb");
    addEntry("Delete",   "To remove or erase permanently",    "Verb");
    addEntry("Elegant",  "Pleasingly graceful and stylish",   "Adjective");
    addEntry("Apple",    "Duplicate — should be rejected",    "Noun");  // duplicate

    // ── Search ───────────────────────────────────────────────
    cout << "\n";
    auto testSearch = [&](const string& w) {
        auto opt = bst.search(w);
        if (opt) {
            const WordEntry& e = opt->get();
            cout << Color::GREEN << "  [FOUND]  " << Color::RESET
                 << "'" << e.word << "'  [" << e.wordType << "]  "
                 << Color::DIM << "— " << e.definition << Color::RESET << "\n";
        } else {
            cout << Color::RED << "  [MISS]   " << Color::RESET
                 << "'" << w << "' not in BST.\n";
        }
    };

    testSearch("compile");    // should find
    testSearch("elegant");    // should find
    testSearch("Zebra");      // should miss

    // ── Delete ───────────────────────────────────────────────
    cout << "\n";
    auto testDelete = [&](const string& w) {
        bool ok = bst.remove(w);
        cout << (ok ? Color::GREEN + "  [DELETE] " : Color::RED + "  [DELETE] ")
             << Color::RESET << "'" << w << "'  → "
             << (ok ? Color::GREEN + "REMOVED" : Color::RED + "NOT FOUND")
             << Color::RESET << "\n";
    };

    testDelete("Binary");     // remove two-child scenario
    testDelete("Apple");      // remove leaf
    testDelete("Phantom");    // not present — should report NOT FOUND

    // Verify deletions
    cout << "\n";
    testSearch("Binary");     // should now miss
    testSearch("Apple");      // should now miss
    testSearch("Compile");    // should still be found

    cout << Color::CYAN << "\n  ══ End of Harika BST Demo ══\n\n" << Color::RESET;
    return 0;
}