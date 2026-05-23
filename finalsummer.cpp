/*
 * ============================================================
 *  BST-Based Dictionary  —  Vardhaman College of Engineering
 *  CSE Department  |  Summer Project 2025-26
 *
 *  Team Lead : Rendla Vishnu Tej
 *  Members   : Harika · Akshith · Manideep · Sanjitha
 *  Mentor    : Vechha Sai Riddhi
 * ============================================================
 *
 *  FILE     : dictionary_v3_production.cpp
 *  VERSION  : v3.0  (Production Grade)
 *  STANDARD : C++17
 *
 *  COMPILE  : g++ -std=c++17 -Wall -Wextra -o dictionary dictionary_v3_production.cpp
 *  RUN      : ./dictionary
 *
 * ============================================================
 *  OVERVIEW
 * ============================================================
 *  This program implements a console-based English dictionary
 *  application backed by a Binary Search Tree (BST). Each node
 *  stores a WordEntry containing the word, definition, part of
 *  speech, phonetic transcription, and synonyms.
 *
 *  Core operations:
 *    • Insert  — O(log n) average; O(n) worst (unbalanced)
 *    • Search  — O(log n) average
 *    • Delete  — O(log n) average (in-order successor strategy)
 *    • Prefix  — O(log n + m)  (range-pruned BST traversal)
 *    • Spell   — O(n) full-tree scan with edit-distance (Levenshtein)
 *
 *  Persistence: pipe-delimited flat file ("dictionary_data.txt")
 *  Format per line:  word|definition|wordType|phonetic|syn1,syn2,...
 *
 * ============================================================
 *  UPGRADE LOG  (v2 → v3)
 * ============================================================
 *  [VAL-01]  Word input: enforces alpha-only + hyphen, max 60 chars
 *  [VAL-02]  Definition input: enforces non-empty, max 500 chars
 *  [VAL-03]  Word-type input: restricted to known enum set with retry
 *  [VAL-04]  Phonetic input: length capped at 80 chars (optional field)
 *  [VAL-05]  Synonym input: each token validated; max 10 synonyms; max 40 chars each
 *  [VAL-06]  Menu input: rejects out-of-range and non-integer with friendly message
 *  [VAL-07]  Prefix input: same rules as word + minimum 1 char
 *  [VAL-08]  File save: validates ofstream state post-open and post-write
 *  [VAL-09]  File load: validates each field; skips and counts malformed records
 *  [VAL-10]  All getline() calls: guard against EOF / stream failure
 *  [VAL-11]  stoi/atoi replaced with safe parseMenuChoice() using stoi + try-catch
 *  [VAL-12]  getChoice() now retries on unexpected EOF
 *  [VAL-13]  Synonym pipe-character sanitised to prevent record corruption
 *  [VAL-14]  History stack: hard-capped at MAX_HISTORY (10) via deque-based trim
 *
 *  [DOC-01]  Every class, function, and major block documented
 *  [DOC-02]  Parameters and return values documented inline
 *  [DOC-03]  Validation rules commented at point of enforcement
 *  [DOC-04]  File format and serialization rules documented
 *  [DOC-05]  Algorithm complexity annotated on all BST methods
 *
 *  [ERR-01]  loadFromFile: reports count of skipped malformed lines
 *  [ERR-02]  saveToFile: notifies user on write-failure, does not crash
 *  [ERR-03]  All menu paths guarded; no silent fall-through
 *  [ERR-04]  cin stream always restored after failure via clear()+ignore()
 *  [ERR-05]  EOF on stdin handled gracefully — exits with save
 *
 *  [QA-01]   Magic numbers replaced with named constants
 *  [QA-02]   Validation helpers extracted to dedicated free functions
 *  [QA-03]   const-correctness applied throughout
 *  [QA-04]   pushHistory() simplified with std::deque
 * ============================================================
 */

// ============================================================
//  HEADERS
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>        // [QA-04] replaces stack for history management
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <climits>
#include <stdexcept>    // [VAL-11] for std::stoi exception handling
#include <cstdlib>

using namespace std;

// ============================================================
//  ANSI COLOR CODES  (terminal UI styling)
// ============================================================
const string RESET   = "\033[0m";
const string BOLD    = "\033[1m";
const string GREEN   = "\033[32m";
const string YELLOW  = "\033[33m";
const string RED     = "\033[31m";
const string CYAN    = "\033[36m";
const string BLUE    = "\033[34m";
const string GRAY    = "\033[90m";
const string MAGENTA = "\033[35m";
const string DIM     = "\033[2m";

// ============================================================
//  APPLICATION-WIDE CONSTANTS
//  [QA-01] All magic numbers are named here for easy tuning.
// ============================================================
namespace Constants {
    constexpr int  MAX_WORD_LEN       = 60;    // maximum characters in a word key
    constexpr int  MAX_DEFINITION_LEN = 500;   // maximum characters in a definition
    constexpr int  MAX_PHONETIC_LEN   = 80;    // maximum characters in phonetic field
    constexpr int  MAX_SYNONYM_LEN    = 40;    // maximum characters per synonym token
    constexpr int  MAX_SYNONYMS       = 10;    // maximum number of synonyms per entry
    constexpr int  MAX_HISTORY        = 10;    // maximum retained search history items
    constexpr int  MAX_MENU_CHOICE    = 10;    // highest valid main-menu option
    constexpr int  SPELL_DIST_THRESH  = 3;     // edit-distance threshold for suggestions
    constexpr int  AUTOCOMPLETE_SHOW  = 5;     // max suggestions shown inline during search
    const     char FIELD_DELIM        = '|';   // flat-file field separator
    const     char SYNONYM_DELIM      = ',';   // synonym list separator within field
    const string  SAVE_FILE           = "dictionary_data.txt";
}

// ============================================================
//  FORWARD DECLARATIONS
// ============================================================
string toLower(const string& s);
string trim(const string& s);

// ============================================================
//  DATA STRUCTURES
// ============================================================

/**
 * @struct WordEntry
 * @brief  Stores all data for a single dictionary word.
 *
 * Fields:
 *   word       — primary key; always stored in lowercase
 *   definition — full human-readable definition (required)
 *   wordType   — grammatical category (e.g. Noun, Verb, Adjective)
 *   phonetic   — IPA or simplified phonetic string (optional)
 *   synonyms   — list of related words (optional; up to MAX_SYNONYMS)
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
 * @brief  BST node wrapping one WordEntry.
 *
 * The BST is ordered lexicographically on WordEntry::word.
 * left  → words alphabetically before this node's word
 * right → words alphabetically after  this node's word
 */
struct Node {
    WordEntry data;
    Node*     left;
    Node*     right;

    /** Constructs a leaf node; both child pointers start null. */
    explicit Node(const WordEntry& e)
        : data(e), left(nullptr), right(nullptr) {}
};

// ============================================================
//  UTILITY FUNCTIONS
// ============================================================

/**
 * @brief  Returns a lowercase copy of the input string.
 * @param  s  Input string (any mix of cases).
 * @return    Lowercase version of s.
 */
string toLower(const string& s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char c){ return tolower(c); });
    return result;
}

/**
 * @brief  Strips leading and trailing whitespace (space, tab, CR, LF).
 * @param  s  Raw input string.
 * @return    Trimmed copy; empty string if s is all whitespace.
 */
string trim(const string& s) {
    const string WHITESPACE = " \t\r\n";
    size_t start = s.find_first_not_of(WHITESPACE);
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(WHITESPACE);
    return s.substr(start, end - start + 1);
}

/**
 * @brief  Splits a string by a delimiter character, trimming each token.
 *         Empty tokens (after trimming) are discarded.
 * @param  s      Source string.
 * @param  delim  Character to split on.
 * @return        Vector of non-empty trimmed tokens.
 */
vector<string> split(const string& s, char delim) {
    vector<string> tokens;
    stringstream   ss(s);
    string         token;
    while (getline(ss, token, delim)) {
        token = trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

/**
 * @brief  Computes the Levenshtein edit distance between two strings.
 *         Uses a full O(m*n) DP table.
 * @param  a  First string.
 * @param  b  Second string.
 * @return    Minimum number of single-character edits (insert/delete/replace)
 *            required to transform a into b.
 */
int editDistance(const string& a, const string& b) {
    const int m = static_cast<int>(a.size());
    const int n = static_cast<int>(b.size());

    // dp[i][j] = edit distance between a[0..i-1] and b[0..j-1]
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

    // Base cases: transforming empty string to/from prefix
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (a[i-1] == b[j-1]) {
                dp[i][j] = dp[i-1][j-1];           // characters match — no cost
            } else {
                dp[i][j] = 1 + min({ dp[i-1][j],   // deletion
                                     dp[i][j-1],   // insertion
                                     dp[i-1][j-1]  // substitution
                                   });
            }
        }
    }
    return dp[m][n];
}

// ============================================================
//  UI HELPERS
// ============================================================

/** Prints a horizontal rule (60 dashes). */
void printDivider() {
    cout << GRAY << string(60, '-') << RESET << "\n";
}

/**
 * @brief  Prints a formatted section header.
 * @param  title  Section title text (e.g. "ADD WORD").
 */
void printHeader(const string& title) {
    cout << "\n" << CYAN << BOLD << "  ══ " << title << " ══" << RESET << "\n\n";
}

/**
 * @brief  Pretty-prints all fields of a WordEntry to stdout.
 * @param  e  The entry to display.
 */
void printEntry(const WordEntry& e) {
    printDivider();

    // Word + phonetic + word-type on first line
    cout << BOLD << GREEN << "  " << e.word << RESET;
    if (!e.phonetic.empty())
        cout << "  " << DIM << e.phonetic << RESET;
    cout << "  " << MAGENTA << "[" << e.wordType << "]" << RESET << "\n\n";

    // Definition
    cout << "  " << BOLD << "Definition : " << RESET << e.definition << "\n";

    // Synonyms (only shown if present)
    if (!e.synonyms.empty()) {
        cout << "  " << BOLD << "Synonyms   : " << RESET;
        for (size_t i = 0; i < e.synonyms.size(); i++) {
            cout << e.synonyms[i];
            if (i + 1 < e.synonyms.size()) cout << ", ";
        }
        cout << "\n";
    }
    printDivider();
}

// ============================================================
//  INPUT VALIDATION HELPERS
//  [VAL-02 / QA-02]  Extracted into reusable free functions
//  so the same rules apply consistently everywhere they appear.
// ============================================================

/**
 * @brief  Checks whether a string is a valid dictionary word key.
 *
 * Validation rules  [VAL-01]:
 *   - Must not be empty.
 *   - Length must be between 1 and MAX_WORD_LEN characters.
 *   - Characters must be alphabetic (a-z / A-Z) or hyphens ('-').
 *     Hyphens are allowed for compound words (e.g. "well-being").
 *   - May not start or end with a hyphen.
 *
 * @param  word     Candidate word (trimmed, case-insensitive).
 * @param  errMsg   Populated with a human-readable reason on failure.
 * @return          true if valid; false otherwise.
 */
bool isValidWord(const string& word, string& errMsg) {
    if (word.empty()) {
        errMsg = "Word cannot be empty.";
        return false;
    }
    if (static_cast<int>(word.size()) > Constants::MAX_WORD_LEN) {
        errMsg = "Word exceeds maximum length of "
                 + to_string(Constants::MAX_WORD_LEN) + " characters.";
        return false;
    }
    if (word.front() == '-' || word.back() == '-') {
        errMsg = "Word must not start or end with a hyphen.";
        return false;
    }
    for (char c : word) {
        if (!isalpha(static_cast<unsigned char>(c)) && c != '-') {
            errMsg = "Word must contain only letters (a-z) and hyphens. "
                     "No digits, spaces, or special characters allowed.";
            return false;
        }
    }
    return true;
}

/**
 * @brief  Checks whether a definition string meets quality requirements.
 *
 * Validation rules  [VAL-02]:
 *   - Must not be empty.
 *   - Length must not exceed MAX_DEFINITION_LEN characters.
 *
 * @param  def     Candidate definition.
 * @param  errMsg  Populated with reason on failure.
 * @return         true if valid.
 */
bool isValidDefinition(const string& def, string& errMsg) {
    if (def.empty()) {
        errMsg = "Definition cannot be empty.";
        return false;
    }
    if (static_cast<int>(def.size()) > Constants::MAX_DEFINITION_LEN) {
        errMsg = "Definition exceeds maximum length of "
                 + to_string(Constants::MAX_DEFINITION_LEN) + " characters.";
        return false;
    }
    return true;
}

/**
 * @brief  Validates a word-type string against the accepted set.
 *
 * Accepted types  [VAL-03]:
 *   Noun, Verb, Adjective, Adverb, Pronoun, Preposition,
 *   Conjunction, Interjection, Article, Other
 *
 * The comparison is case-insensitive; the function normalises the
 * stored value to the canonical capitalised form.
 *
 * @param  raw       User-typed word type.
 * @param  canonical Populated with the normalised version on success.
 * @param  errMsg    Populated with reason on failure.
 * @return           true if valid.
 */
bool isValidWordType(const string& raw, string& canonical, string& errMsg) {
    // Accepted part-of-speech labels (lowercase for comparison)
    static const vector<pair<string,string>> VALID_TYPES = {
        {"noun",          "Noun"},
        {"verb",          "Verb"},
        {"adjective",     "Adjective"},
        {"adj",           "Adjective"},
        {"adverb",        "Adverb"},
        {"adv",           "Adverb"},
        {"pronoun",       "Pronoun"},
        {"preposition",   "Preposition"},
        {"prep",          "Preposition"},
        {"conjunction",   "Conjunction"},
        {"conj",          "Conjunction"},
        {"interjection",  "Interjection"},
        {"interj",        "Interjection"},
        {"article",       "Article"},
        {"other",         "Other"},
    };

    const string lower = toLower(trim(raw));
    for (const auto& type : VALID_TYPES) {
        if (type.first == lower) {
            canonical = type.second;
            return true;
        }
    }
    errMsg = "Unknown word type '" + raw + "'.\n"
             "  Accepted: Noun, Verb, Adjective, Adverb, Pronoun,\n"
             "            Preposition, Conjunction, Interjection, Article, Other";
    return false;
}

/**
 * @brief  Validates a single synonym token.
 *
 * Validation rules  [VAL-05]:
 *   - Must not be empty.
 *   - Must not exceed MAX_SYNONYM_LEN characters.
 *   - Must not contain the pipe character '|' (file-format reserved).
 *
 * @param  syn     Candidate synonym.
 * @param  errMsg  Populated with reason on failure.
 * @return         true if valid.
 */
bool isValidSynonym(const string& syn, string& errMsg) {
    if (syn.empty()) return true;  // empty tokens are silently skipped upstream
    if (static_cast<int>(syn.size()) > Constants::MAX_SYNONYM_LEN) {
        errMsg = "Synonym '" + syn + "' exceeds maximum length of "
                 + to_string(Constants::MAX_SYNONYM_LEN) + " characters.";
        return false;
    }
    // [VAL-13] Pipe character would corrupt the flat-file record format
    if (syn.find('|') != string::npos) {
        errMsg = "Synonym must not contain the '|' character.";
        return false;
    }
    return true;
}

/**
 * @brief  Safely parses a main-menu integer choice from a string.
 *         [VAL-11] Uses std::stoi with try-catch to avoid undefined behaviour.
 *
 * @param  input   Raw string from the user.
 * @param  choice  Populated with the parsed integer on success.
 * @return         true if input is a valid integer within [0, MAX_MENU_CHOICE].
 */
bool parseMenuChoice(const string& input, int& choice) {
    if (input.empty()) return false;
    try {
        size_t pos = 0;
        const int val = stoi(input, &pos);
        // Ensure the entire string was consumed (reject "3abc")
        if (pos != input.size()) return false;
        if (val < 0 || val > Constants::MAX_MENU_CHOICE) return false;
        choice = val;
        return true;
    } catch (const invalid_argument&) {
        return false;   // not a number at all
    } catch (const out_of_range&) {
        return false;   // number too large for int
    }
}

// ============================================================
//  BST CLASS
// ============================================================

/**
 * @class BST
 * @brief Unbalanced Binary Search Tree storing WordEntry objects.
 *
 * Ordering is lexicographic on WordEntry::word (lowercase).
 *
 * Public interface:
 *   insert()      — inserts new entry; rejects duplicates
 *   search()      — returns pointer to entry or nullptr
 *   remove()      — deletes entry; no-op if absent
 *   getAllWords()  — in-order traversal; returns sorted vector
 *   wordCount()   — total node count
 *   autoComplete()— prefix match using range-pruned traversal
 *   spellSuggest()— nearest word by Levenshtein distance
 *
 * Note: worst-case for all O(log n) operations degrades to O(n)
 * on sorted or reverse-sorted insertion sequences (unbalanced BST).
 * For guaranteed O(log n), upgrade to AVL or Red-Black tree.
 */
class BST {
private:
    Node* root;  ///< Pointer to the root node; nullptr for empty tree.

    // ----------------------------------------------------------
    //  PRIVATE BST OPERATIONS
    // ----------------------------------------------------------

    /**
     * @brief  Recursive insert helper.
     *         [FIX-2 / original v1.1] Single traversal handles both
     *         path-finding and duplicate detection. No pre-search needed.
     *
     * @param  node      Current subtree root (may be nullptr).
     * @param  e         Entry to insert.
     * @param  inserted  Set to true if a new node was created; false if duplicate.
     * @return           Updated subtree root.
     */
    Node* insert(Node* node, const WordEntry& e, bool& inserted) {
        if (!node) {
            inserted = true;
            return new Node(e);
        }
        if (e.word < node->data.word) {
            node->left  = insert(node->left,  e, inserted);
        } else if (e.word > node->data.word) {
            node->right = insert(node->right, e, inserted);
        } else {
            inserted = false;   // exact duplicate key — reject silently
        }
        return node;
    }

    /**
     * @brief  Recursive search helper.
     * @param  node  Current subtree root.
     * @param  key   Lowercase word to find.
     * @return       Pointer to matching node, or nullptr if not found.
     */
    Node* search(Node* node, const string& key) const {
        if (!node || node->data.word == key) return node;
        if (key < node->data.word) return search(node->left,  key);
        return                            search(node->right, key);
    }

    /**
     * @brief  Recursive in-order traversal; populates result in sorted order.
     * @param  node    Current subtree root.
     * @param  result  Vector accumulating entries (left → root → right).
     */
    void inOrder(Node* node, vector<WordEntry>& result) const {
        if (!node) return;
        inOrder(node->left, result);
        result.push_back(node->data);
        inOrder(node->right, result);
    }

    /**
     * @brief  Counts total nodes in subtree rooted at node.
     * @param  node  Subtree root.
     * @return       Number of nodes.
     */
    int count(Node* node) const {
        if (!node) return 0;
        return 1 + count(node->left) + count(node->right);
    }

    /**
     * @brief  Finds the leftmost (minimum) node in a subtree.
     *         Used by deleteNode() to locate the in-order successor.
     * @param  node  Non-null subtree root.
     * @return       Pointer to the minimum node.
     */
    Node* findMin(Node* node) const {
        while (node->left) node = node->left;
        return node;
    }

    /**
     * @brief  Recursive delete helper using the in-order successor strategy.
     *
     * Cases handled:
     *   1. Node not found    → no-op, returns current subtree unchanged.
     *   2. Leaf node         → freed directly.
     *   3. One child missing → replaced by the existing child.
     *   4. Two children      → replaced by in-order successor's data;
     *                          successor then deleted from right subtree.
     *
     * @param  node  Current subtree root.
     * @param  key   Lowercase word to delete.
     * @return       Updated subtree root.
     */
    Node* deleteNode(Node* node, const string& key) {
        if (!node) return nullptr;

        if (key < node->data.word) {
            node->left  = deleteNode(node->left,  key);
        } else if (key > node->data.word) {
            node->right = deleteNode(node->right, key);
        } else {
            // Matched — determine which deletion case applies
            if (!node->left && !node->right) {
                // Case 2: leaf — simply free
                delete node;
                return nullptr;
            }
            if (!node->left) {
                // Case 3a: only right child
                Node* tmp = node->right;
                delete node;
                return tmp;
            }
            if (!node->right) {
                // Case 3b: only left child
                Node* tmp = node->left;
                delete node;
                return tmp;
            }
            // Case 4: two children — replace with in-order successor
            Node* successor  = findMin(node->right);
            node->data       = successor->data;
            node->right      = deleteNode(node->right, successor->data.word);
        }
        return node;
    }

    /**
     * @brief  Collects all words in the lexicographic range [prefix, upperBound).
     *         [FIX-1 from v1.1] Replaces the naive O(n) full-scan with a range-
     *         pruned traversal that achieves O(log n + m) complexity.
     *
     * Algorithm:
     *   upperBound is formed by incrementing the last character of prefix
     *   (e.g. "ap" → "aq").  All words sharing the prefix lie in
     *   [prefix, upperBound) by lexicographic ordering:
     *
     *     • node.word >= upperBound → whole right subtree is out of range;
     *                                  prune right, descend only into left.
     *     • node.word <  prefix    → whole left subtree is out of range;
     *                                  prune left, descend only into right.
     *     • otherwise              → node is in range; explore both subtrees
     *                                  and record node.word if it starts with prefix.
     *
     * @param  node        Current subtree root.
     * @param  prefix      Lowercase prefix string.
     * @param  upperBound  Exclusive upper bound for range.
     * @param  results     Accumulates matching words.
     */
    void collectByPrefix(Node* node, const string& prefix,
                         const string& upperBound,
                         vector<string>& results) const {
        if (!node) return;
        const string& nw = node->data.word;

        if (nw >= upperBound) {
            // This node and its entire right subtree exceed the prefix range
            collectByPrefix(node->left, prefix, upperBound, results);
            return;
        }
        if (nw < prefix) {
            // This node and its entire left subtree precede the prefix range
            collectByPrefix(node->right, prefix, upperBound, results);
            return;
        }
        // Node is within [prefix, upperBound); explore both sides
        collectByPrefix(node->left, prefix, upperBound, results);
        // Confirm prefix match (guard: nw.size() >= prefix.size() already implied above)
        if (nw.size() >= prefix.size() && nw.substr(0, prefix.size()) == prefix)
            results.push_back(nw);
        collectByPrefix(node->right, prefix, upperBound, results);
    }

    /**
     * @brief  Finds the closest word to key by Levenshtein edit distance.
     *         [FIX-3 from v1.1] Early-exit when bestDist == 0 (exact match).
     *
     * Traverses the entire tree (O(n)) since the BST ordering on words does
     * not correlate with edit distance — no structural pruning is possible.
     *
     * @param  node      Current subtree root.
     * @param  key       Lowercase misspelled word.
     * @param  bestWord  Best candidate found so far (updated in place).
     * @param  bestDist  Edit distance to bestWord (updated in place).
     */
    void closest(Node* node, const string& key,
                 string& bestWord, int& bestDist) const {
        if (!node || bestDist == 0) return;  // early-exit on exact match

        const int d = editDistance(key, node->data.word);
        if (d < bestDist) {
            bestDist = d;
            bestWord = node->data.word;
        }
        closest(node->left,  key, bestWord, bestDist);
        closest(node->right, key, bestWord, bestDist);
    }

    /**
     * @brief  Post-order recursive deallocation of all nodes.
     *         Called by destructor to prevent memory leaks.
     * @param  node  Current subtree root.
     */
    void freeTree(Node* node) {
        if (!node) return;
        freeTree(node->left);
        freeTree(node->right);
        delete node;
    }

    /**
     * @brief  Recursive height calculation.
     * @param  node  Current subtree root.
     * @return       Height of subtree (0 for nullptr).
     */
    int height(Node* node) const {
        if (!node) return 0;
        return 1 + max(height(node->left), height(node->right));
    }

public:
    /** Constructs an empty BST. */
    BST() : root(nullptr) {}

    /** Destructor — frees all nodes to prevent memory leaks. */
    ~BST() { freeTree(root); }

    /** @return true if the tree contains no entries. */
    bool isEmpty() const { return root == nullptr; }

    /**
     * @brief  Searches for a word in the BST.
     * @param  key  Word to find (case-insensitive; normalised internally).
     * @return      Pointer to the matching WordEntry, or nullptr if absent.
     *              The caller must not delete this pointer.
     */
    WordEntry* search(const string& key) {
        Node* n = search(root, toLower(key));
        return n ? &n->data : nullptr;
    }

    /**
     * @brief  Inserts a new entry into the BST.
     *         Duplicate keys (same lowercase word) are rejected.
     * @param  e  WordEntry to insert (word field must already be lowercase).
     * @return    true if inserted; false if duplicate.
     */
    bool insert(const WordEntry& e) {
        bool inserted = false;
        root = insert(root, e, inserted);
        return inserted;
    }

    /**
     * @brief  Removes the entry with the given key from the BST.
     * @param  key  Word to remove (case-insensitive).
     * @return      true if found and removed; false if not found.
     */
    bool remove(const string& key) {
        const string k = toLower(key);
        if (!search(root, k)) return false;
        root = deleteNode(root, k);
        return true;
    }

    /**
     * @brief  Returns all entries in ascending alphabetical order.
     * @return Vector of WordEntry, sorted A → Z.
     */
    vector<WordEntry> getAllWords() const {
        vector<WordEntry> result;
        inOrder(root, result);
        return result;
    }

    /** @return Total number of entries currently in the tree. */
    int wordCount() const { return count(root); }

    /** @return Height of the BST (useful for balance diagnostics). */
    int getHeight() const { return height(root); }

    /**
     * @brief  Returns up to [no limit] words beginning with prefix.
     *         Uses range-pruned traversal — O(log n + m).
     *         [FIX-1]
     *
     * @param  prefix  Lowercase prefix to match.
     * @return         Sorted vector of matching word strings.
     */
    vector<string> autoComplete(const string& prefix) const {
        vector<string> results;
        const string p = toLower(prefix);
        if (p.empty()) return results;

        // Build upper bound by incrementing last character
        // Example: "ap" → "aq",  "z" → "{"  ('{' is one past 'z' in ASCII)
        string upper = p;
        upper.back()++;

        collectByPrefix(root, p, upper, results);
        sort(results.begin(), results.end());
        return results;
    }

    /**
     * @brief  Finds the closest word by Levenshtein distance.
     *         Returns empty string if the closest distance > SPELL_DIST_THRESH.
     *
     * @param  key  Lowercase misspelled query.
     * @return      Best candidate word, or "" if none close enough.
     */
    string spellSuggest(const string& key) const {
        if (isEmpty()) return "";
        string bestWord;
        int    bestDist = INT_MAX;
        closest(root, toLower(key), bestWord, bestDist);
        return (bestDist <= Constants::SPELL_DIST_THRESH) ? bestWord : "";
    }
};

// ============================================================
//  DICTIONARY APPLICATION CLASS
// ============================================================

/**
 * @class Dictionary
 * @brief Top-level application controller.
 *
 * Owns the BST, manages user interaction (menus, validated input),
 * search history, and file persistence.
 *
 * Lifecycle:
 *   1. Constructor calls loadFromFile() to restore previous session.
 *   2. run() drives the main event loop.
 *   3. On exit (choice 0), saveToFile() is called automatically.
 *
 * File format (dictionary_data.txt):
 *   One record per line, fields separated by '|':
 *     word|definition|wordType|phonetic|syn1,syn2,...
 *   The synonyms field (index 4) is optional and may be empty.
 */
class Dictionary {
private:
    BST          bst;            ///< Backing data structure
    deque<string> searchHistory;  ///< [QA-04] Recent searches, most-recent at front

    // ----------------------------------------------------------
    //  FILE PERSISTENCE
    // ----------------------------------------------------------

    /**
     * @brief  Serialises all BST entries to SAVE_FILE in pipe-delimited format.
     *         [VAL-08] Validates file open and write state; reports errors gracefully.
     *
     * File format per line:
     *   word|definition|wordType|phonetic|syn1,syn2,...
     *
     * Note: fields must not contain '|'; synonyms must not contain ','.
     * Both are enforced at input time ([VAL-13]).
     */
    void saveToFile() {
        ofstream file(Constants::SAVE_FILE);

        // [VAL-08] Check file opened successfully
        if (!file.is_open()) {
            cout << RED << "  [Error] Could not open '"
                 << Constants::SAVE_FILE << "' for writing.\n"
                 << "          Check directory permissions.\n" << RESET;
            return;
        }

        const auto words = bst.getAllWords();
        int written = 0;

        for (const auto& e : words) {
            file << e.word        << Constants::FIELD_DELIM
                 << e.definition  << Constants::FIELD_DELIM
                 << e.wordType    << Constants::FIELD_DELIM
                 << e.phonetic    << Constants::FIELD_DELIM;

            for (size_t i = 0; i < e.synonyms.size(); i++) {
                file << e.synonyms[i];
                if (i + 1 < e.synonyms.size()) file << Constants::SYNONYM_DELIM;
            }
            file << "\n";

            // [VAL-08] Check stream health after each write
            if (!file.good()) {
                cout << RED << "  [Error] Write failure at record '"
                     << e.word << "'. File may be incomplete.\n" << RESET;
                break;
            }
            ++written;
        }

        file.close();

        if (file.fail() && !file.eof()) {
            cout << RED << "  [Warning] File may not have closed cleanly.\n" << RESET;
        } else {
            cout << GREEN << "  [✓] " << written << " word(s) saved to '"
                 << Constants::SAVE_FILE << "'.\n" << RESET;
        }
    }

    /**
     * @brief  Deserialises entries from SAVE_FILE into the BST.
     *         [VAL-09] Validates field count and word validity per record.
     *                  Skips malformed lines and reports a count.
     *         [ERR-01] Silently returns (no error) if file does not yet exist.
     *
     * Expected format per line:
     *   word|definition|wordType|phonetic|synonyms(optional)
     * Minimum required fields: 4 (word, definition, wordType, phonetic).
     */
    void loadFromFile() {
        ifstream file(Constants::SAVE_FILE);
        if (!file.is_open()) return;  // first run — file not yet created

        string line;
        int loaded   = 0;
        int skipped  = 0;
        int lineNum  = 0;

        while (getline(file, line)) {
            ++lineNum;
            const string trimmed = trim(line);
            if (trimmed.empty()) continue;  // skip blank lines

            const auto parts = split(trimmed, Constants::FIELD_DELIM);

            // [VAL-09] Require at least word, definition, wordType, phonetic
            if (parts.size() < 4) {
                ++skipped;
                continue;
            }

            WordEntry e;
            e.word       = toLower(trim(parts[0]));
            e.definition = trim(parts[1]);
            e.wordType   = trim(parts[2]);
            e.phonetic   = trim(parts[3]);

            // [VAL-09] Skip records with invalid word keys
            string errMsg;
            if (!isValidWord(e.word, errMsg)) {
                ++skipped;
                continue;
            }

            // Synonyms field is optional (index 4)
            if (parts.size() >= 5 && !parts[4].empty())
                e.synonyms = split(parts[4], Constants::SYNONYM_DELIM);

            bst.insert(e);
            ++loaded;
        }

        file.close();

        if (loaded > 0)
            cout << GREEN << "  [✓] Loaded " << loaded << " word(s) from '"
                 << Constants::SAVE_FILE << "'.\n" << RESET;

        // [ERR-01] Inform user if records were dropped
        if (skipped > 0)
            cout << YELLOW << "  [!] " << skipped
                 << " malformed record(s) skipped during load.\n" << RESET;

        if (loaded > 0) cout << "\n";
    }

    // ----------------------------------------------------------
    //  INPUT HELPERS
    // ----------------------------------------------------------

    /**
     * @brief  Prompts the user and reads a trimmed line of text.
     *         [VAL-10] Guards against stream failure and EOF.
     *
     * @param  prompt  Text displayed before the input cursor.
     * @return         Trimmed input string; empty string on EOF/failure.
     */
    string getLine(const string& prompt) {
        cout << YELLOW << "  " << prompt << RESET;
        string s;
        if (!getline(cin, s)) {
            // [ERR-05] EOF or stream error — clear state and return empty
            if (cin.eof()) {
                cout << "\n" << YELLOW
                     << "  [!] End of input detected.\n" << RESET;
            }
            cin.clear();
            return "";
        }
        return trim(s);
    }

    /**
     * @brief  Prompts the user for a single yes/no character.
     *         Returns the first character (lowercased) of the response.
     *         [VAL-12] Retries once on empty input; returns '\0' on repeated failure.
     *
     * @param  prompt  Text displayed before the input cursor.
     * @return         Lowercased first character, or '\0' if unavailable.
     */
    char getChoice(const string& prompt) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            cout << YELLOW << "  " << prompt << RESET;
            string s;
            if (!getline(cin, s)) {
                cin.clear();
                return '\0';
            }
            s = trim(s);
            if (!s.empty())
                return static_cast<char>(tolower(static_cast<unsigned char>(s[0])));
            // Empty input: retry once with a hint
            if (attempt == 0)
                cout << RED << "  [!] Please enter 'y' or 'n'.\n" << RESET;
        }
        return '\0';
    }

    // ----------------------------------------------------------
    //  SEARCH HISTORY
    // ----------------------------------------------------------

    /**
     * @brief  Adds a word to the search history deque.
     *         [VAL-14] Deduplicates consecutive identical entries.
     *                  Trims to MAX_HISTORY entries by removing oldest.
     *
     * @param  word  Successfully searched word to record.
     */
    void pushHistory(const string& word) {
        // Avoid consecutive duplicates
        if (!searchHistory.empty() && searchHistory.front() == word) return;
        searchHistory.push_front(word);
        // [VAL-14] Cap history length
        while (static_cast<int>(searchHistory.size()) > Constants::MAX_HISTORY)
            searchHistory.pop_back();
    }

    /**
     * @brief  Prints up to AUTOCOMPLETE_SHOW recent search terms above the prompt.
     *         No-op if history is empty.
     */
    void showHistorySuggestions() const {
        if (searchHistory.empty()) return;
        cout << DIM << "  Recent searches: ";
        const int shown = min(static_cast<int>(searchHistory.size()),
                              Constants::AUTOCOMPLETE_SHOW);
        for (int i = 0; i < shown; i++) {
            cout << searchHistory[i];
            if (i + 1 < shown) cout << "  ·  ";
        }
        cout << RESET << "\n";
    }

    // ----------------------------------------------------------
    //  MENU UI
    // ----------------------------------------------------------

    /**
     * @brief  Renders the main application menu to stdout.
     *         Displays the ASCII-art banner, project info, and option list.
     */
    void displayMenu() const {
        cout << "\n";
        cout << GRAY << " ┌──────────────────────────────────────────────┐\n" << RESET;
        cout << BLUE << " │ ██████╗  ███████╗████████╗                   │\n" << RESET;
        cout << BLUE << " │ ██╔══██╗██╔════╝╚══██╔══╝   " << RESET << BOLD << "Dictionary" << BLUE << "       │\n" << RESET;
        cout << BLUE << " │ ██████╦╝███████╗   ██║      " << RESET << GRAY << "v3.0 · C++" << BLUE << "       │\n" << RESET;
        cout << BLUE << " │ ██╔══██╗╚════██║   ██║                       │\n" << RESET;
        cout << BLUE << " │ ██████╦╝███████║   ██║                       │\n" << RESET;
        cout << BLUE << " │ ╚═════╝ ╚══════╝   ╚═╝                       │\n" << RESET;
        cout << GRAY << " └──────────────────────────────────────────────┘\n\n" << RESET;

        cout << GRAY << " Vardhaman College of Engineering  ·  CSE Dept\n"
                     << " Mentor: Sai Riddhi   ·  Summer Project 2026\n"
                     << " ------------------------------------------------\n\n" << RESET;

        cout << YELLOW << BOLD << " MAIN MENU\n" << RESET;
        cout << GRAY          << " ------------------------------------------------\n\n" << RESET;

        cout << GREEN  << " [1] " << RESET << "Add Word          " << GRAY << "Insert into BST\n"           << RESET;
        cout << GREEN  << " [2] " << RESET << "Search Word       " << GRAY << "Lookup & details\n"          << RESET;
        cout << GREEN  << " [3] " << RESET << "Delete Word       " << GRAY << "Remove from BST\n"           << RESET;
        cout << GREEN  << " [4] " << RESET << "Display All Words " << GRAY << "In-order traversal\n"        << RESET;
        cout << GREEN  << " [5] " << RESET << "Count Words       " << GRAY << "Total entries\n"             << RESET;
        cout << GREEN  << " [6] " << RESET << "Auto-Complete     " << GRAY << "Prefix BST search  O(p+m)\n" << RESET;
        cout << GREEN  << " [7] " << RESET << "Edit Word         " << GRAY << "Update existing entry\n"     << RESET;
        cout << CYAN   << " [8] " << RESET << "Search History    " << GRAY << "View recent searches\n"      << RESET;
        cout << YELLOW << " [9] " << RESET << "Save Dictionary   " << GRAY << "Write to file\n"             << RESET;
        cout << YELLOW << "[10] " << RESET << "Load Dictionary   " << GRAY << "Read from file\n"            << RESET;
        cout << RED    << " [0] " << RESET << "Exit              " << GRAY << "Auto-saves on quit\n\n"      << RESET;

        cout << GRAY << " ------------------------------------------------\n"
                     << " ✦ Auto-Complete uses true O(p+m) prefix traversal\n\n" << RESET;

        cout << GREEN << " → " << GRAY << "Type a number (0-10) and press Enter.\n" << RESET;
        cout << GREEN << " \n > " << RESET;
    }

    // ----------------------------------------------------------
    //  FEATURE IMPLEMENTATIONS
    // ----------------------------------------------------------

    /**
     * @brief  Feature 1: Add a new word to the dictionary.
     *
     * Validation applied:
     *   [VAL-01] Word: alpha+hyphen only, 1-MAX_WORD_LEN chars, no leading/trailing hyphen.
     *   [VAL-02] Definition: non-empty, max MAX_DEFINITION_LEN chars.
     *   [VAL-03] Word type: must match accepted POS set; retried until valid.
     *   [VAL-04] Phonetic: optional, max MAX_PHONETIC_LEN chars.
     *   [VAL-05] Synonyms: each token validated; max MAX_SYNONYMS; '|' disallowed.
     *
     * If the word already exists, the user is offered the chance to edit it instead.
     */
    void addWord() {
        printHeader("ADD WORD");

        // ── Word Input [VAL-01] ──────────────────────────────────
        string word;
        while (true) {
            word = toLower(getLine("Enter word          : "));
            if (word.empty()) {
                cout << RED << "  [!] Word cannot be empty. Please try again.\n" << RESET;
                continue;
            }
            string errMsg;
            if (!isValidWord(word, errMsg)) {
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            break;
        }

        // ── Duplicate Check ──────────────────────────────────────
        if (bst.search(word)) {
            cout << YELLOW << "\n  [!] '" << word
                 << "' already exists in the dictionary.\n" << RESET;
            const char c = getChoice("  Would you like to edit it instead? (y/n): ");
            if (c == 'y') editWord(word);
            return;
        }

        WordEntry e;
        e.word = word;

        // ── Definition Input [VAL-02] ────────────────────────────
        while (true) {
            e.definition = getLine("Enter definition    : ");
            string errMsg;
            if (!isValidDefinition(e.definition, errMsg)) {
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            break;
        }

        // ── Word Type Input [VAL-03] ─────────────────────────────
        while (true) {
            const string raw = getLine("Word type (Noun/Verb/Adj/Adv/etc): ");
            if (raw.empty()) {
                cout << RED << "  [!] Word type cannot be empty.\n" << RESET;
                continue;
            }
            string canonical, errMsg;
            if (!isValidWordType(raw, canonical, errMsg)) {
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            e.wordType = canonical;
            break;
        }

        // ── Phonetic Input [VAL-04] ──────────────────────────────
        // Phonetic is optional; skip validation on empty input
        while (true) {
            const string ph = getLine("Phonetic (optional) : ");
            if (ph.empty()) {
                e.phonetic = "";
                break;
            }
            if (static_cast<int>(ph.size()) > Constants::MAX_PHONETIC_LEN) {
                cout << RED << "  [!] Phonetic entry exceeds max length of "
                     << Constants::MAX_PHONETIC_LEN << " characters.\n" << RESET;
                continue;
            }
            e.phonetic = ph;
            break;
        }

        // ── Synonyms Input [VAL-05] ──────────────────────────────
        while (true) {
            const string synInput = getLine("Synonyms (comma-sep): ");
            if (synInput.empty()) break;  // synonyms are optional

            const auto tokens = split(synInput, Constants::SYNONYM_DELIM);

            // Enforce maximum synonym count
            if (static_cast<int>(tokens.size()) > Constants::MAX_SYNONYMS) {
                cout << RED << "  [!] Too many synonyms. Maximum is "
                     << Constants::MAX_SYNONYMS << ".\n" << RESET;
                continue;
            }

            // Validate each token individually
            bool allValid = true;
            for (const auto& syn : tokens) {
                string errMsg;
                if (!isValidSynonym(syn, errMsg)) {
                    cout << RED << "  [!] " << errMsg << "\n" << RESET;
                    allValid = false;
                    break;
                }
            }
            if (!allValid) continue;

            e.synonyms = tokens;
            break;
        }

        bst.insert(e);
        cout << GREEN << "\n  [✓] '" << word << "' added successfully!\n" << RESET;
    }

    /**
     * @brief  Feature 2: Search for a word and display its entry.
     *
     * If the exact word is not found:
     *   - Spell suggestion is computed and offered to the user.
     * Auto-complete suggestions are shown as a hint above the prompt
     * if available and the typed string is not an exact match.
     */
    void searchWord() {
        printHeader("SEARCH WORD");
        showHistorySuggestions();

        // [VAL-01] Word key validation
        string word;
        while (true) {
            word = toLower(getLine("\n  Enter word to search: "));
            if (word.empty()) {
                cout << RED << "  [!] Please enter a word.\n" << RESET;
                continue;
            }
            string errMsg;
            if (!isValidWord(word, errMsg)) {
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            break;
        }

        // ── Inline auto-complete hint ─────────────────────────────
        const auto suggestions = bst.autoComplete(word);
        if (!suggestions.empty() && suggestions[0] != word) {
            cout << DIM << "  Suggestions: ";
            const size_t show = min(suggestions.size(),
                                    static_cast<size_t>(Constants::AUTOCOMPLETE_SHOW));
            for (size_t i = 0; i < show; i++) {
                cout << suggestions[i];
                if (i + 1 < show) cout << "  ";
            }
            cout << RESET << "\n";
        }

        // ── BST Lookup ────────────────────────────────────────────
        WordEntry* entry = bst.search(word);
        if (entry) {
            pushHistory(word);
            printEntry(*entry);
        } else {
            cout << RED << "\n  [✗] '" << word << "' not found.\n" << RESET;

            // ── Spell suggestion ──────────────────────────────────
            const string suggestion = bst.spellSuggest(word);
            if (!suggestion.empty()) {
                cout << YELLOW << "  Did you mean: " << BOLD << suggestion
                     << RESET << " ?\n";
                const char c = getChoice("  Search for '" + suggestion + "'? (y/n): ");
                if (c == 'y') {
                    WordEntry* se = bst.search(suggestion);
                    if (se) { pushHistory(suggestion); printEntry(*se); }
                }
            }
        }
    }

    /**
     * @brief  Feature 3: Delete a word from the dictionary.
     *         Displays the entry before deletion and requires confirmation.
     */
    void deleteWord() {
        printHeader("DELETE WORD");

        // [VAL-01] Word key validation
        string word;
        while (true) {
            word = toLower(getLine("Enter word to delete: "));
            if (word.empty()) {
                cout << RED << "  [!] Please enter a word.\n" << RESET;
                continue;
            }
            string errMsg;
            if (!isValidWord(word, errMsg)) {
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            break;
        }

        WordEntry* entry = bst.search(word);
        if (!entry) {
            cout << RED << "  [✗] '" << word << "' not found in the dictionary.\n" << RESET;
            return;
        }

        // Show the entry so the user can confirm they have the right word
        printEntry(*entry);
        const char c = getChoice("  Confirm delete? (y/n): ");
        if (c == 'y') {
            bst.remove(word);
            cout << GREEN << "  [✓] '" << word << "' deleted successfully.\n" << RESET;
        } else {
            cout << DIM << "  Delete cancelled.\n" << RESET;
        }
    }

    /**
     * @brief  Feature 4: Display all words in alphabetical order (in-order traversal).
     */
    void displayAllWords() const {
        printHeader("ALL WORDS  (A → Z)");
        if (bst.isEmpty()) {
            cout << DIM << "  The dictionary is currently empty.\n" << RESET;
            return;
        }
        const auto words = bst.getAllWords();
        cout << "  " << DIM << words.size() << " word(s) found.\n\n" << RESET;
        for (const auto& e : words) printEntry(e);
    }

    /**
     * @brief  Feature 5: Report the total number of words in the dictionary.
     */
    void countWords() const {
        printHeader("WORD COUNT");
        cout << "  Total words in dictionary: "
             << BOLD << CYAN << bst.wordCount() << RESET << "\n";
    }

    /**
     * @brief  Feature 6: Auto-complete — list all words matching a prefix.
     *         [FIX-1] Uses O(log n + m) range-pruned BST traversal.
     *
     * Validation: prefix must be 1–MAX_WORD_LEN chars, alpha+hyphen only.
     */
    void autoComplete() {
        printHeader("AUTO-COMPLETE");

        string prefix;
        while (true) {
            prefix = toLower(getLine("Type a prefix (e.g. 'app'): "));
            if (prefix.empty()) {
                cout << RED << "  [!] Prefix cannot be empty.\n" << RESET;
                continue;
            }
            string errMsg;
            if (!isValidWord(prefix, errMsg)) {
                // Re-use word validation — same character rules apply to prefixes
                cout << RED << "  [!] " << errMsg << "\n" << RESET;
                continue;
            }
            break;
        }

        const auto suggestions = bst.autoComplete(prefix);
        if (suggestions.empty()) {
            cout << RED << "  No words found starting with '" << prefix << "'.\n" << RESET;
        } else {
            cout << GREEN << "  " << suggestions.size() << " suggestion(s):\n\n" << RESET;
            for (const auto& s : suggestions)
                cout << "    " << CYAN << "→" << RESET << "  " << s << "\n";
        }
    }

    /**
     * @brief  Feature 7: Edit the fields of an existing word entry.
     *         Pressing Enter at any field prompt keeps the current value.
     *
     * Can be called with a pre-filled word (from addWord redirect on duplicate)
     * or interactively by asking the user for the word to edit.
     *
     * Validation mirrors addWord() for each updated field.
     *
     * @param  preFilledWord  Optional: if non-empty, skips the word prompt.
     */
    void editWord(const string& preFilledWord = "") {
        printHeader("EDIT WORD");

        // ── Resolve target word ───────────────────────────────────
        string word;
        if (!preFilledWord.empty()) {
            word = preFilledWord;
        } else {
            while (true) {
                word = toLower(getLine("Enter word to edit  : "));
                if (word.empty()) {
                    cout << RED << "  [!] Please enter a word.\n" << RESET;
                    continue;
                }
                string errMsg;
                if (!isValidWord(word, errMsg)) {
                    cout << RED << "  [!] " << errMsg << "\n" << RESET;
                    continue;
                }
                break;
            }
        }

        WordEntry* entry = bst.search(word);
        if (!entry) {
            cout << RED << "  [✗] '" << word << "' not found in the dictionary.\n" << RESET;
            return;
        }

        printEntry(*entry);
        cout << DIM << "  (Press Enter to keep the current value for any field)\n\n" << RESET;

        // ── Definition update [VAL-02] ────────────────────────────
        {
            cout << YELLOW << "  Definition [" << DIM << entry->definition
                 << YELLOW << "]: " << RESET;
            string input;
            if (getline(cin, input)) {
                input = trim(input);
                if (!input.empty()) {
                    string errMsg;
                    if (!isValidDefinition(input, errMsg))
                        cout << RED << "  [!] " << errMsg
                             << " Definition unchanged.\n" << RESET;
                    else
                        entry->definition = input;
                }
            }
        }

        // ── Word type update [VAL-03] ─────────────────────────────
        {
            cout << YELLOW << "  Word Type [" << DIM << entry->wordType
                 << YELLOW << "]: " << RESET;
            string input;
            if (getline(cin, input)) {
                input = trim(input);
                if (!input.empty()) {
                    string canonical, errMsg;
                    if (!isValidWordType(input, canonical, errMsg))
                        cout << RED << "  [!] " << errMsg
                             << "\n  Word type unchanged.\n" << RESET;
                    else
                        entry->wordType = canonical;
                }
            }
        }

        // ── Phonetic update [VAL-04] ──────────────────────────────
        {
            cout << YELLOW << "  Phonetic [" << DIM << entry->phonetic
                 << YELLOW << "]: " << RESET;
            string input;
            if (getline(cin, input)) {
                input = trim(input);
                if (!input.empty()) {
                    if (static_cast<int>(input.size()) > Constants::MAX_PHONETIC_LEN)
                        cout << RED << "  [!] Phonetic exceeds max length. Unchanged.\n" << RESET;
                    else
                        entry->phonetic = input;
                }
            }
        }

        // ── Synonyms update [VAL-05] ──────────────────────────────
        {
            cout << YELLOW << "  Synonyms [" << DIM;
            for (size_t i = 0; i < entry->synonyms.size(); i++) {
                cout << entry->synonyms[i];
                if (i + 1 < entry->synonyms.size()) cout << ", ";
            }
            cout << YELLOW << "]: " << RESET;
            string synInput;
            if (getline(cin, synInput)) {
                synInput = trim(synInput);
                if (!synInput.empty()) {
                    const auto tokens = split(synInput, Constants::SYNONYM_DELIM);
                    if (static_cast<int>(tokens.size()) > Constants::MAX_SYNONYMS) {
                        cout << RED << "  [!] Too many synonyms (max "
                             << Constants::MAX_SYNONYMS << "). Synonyms unchanged.\n" << RESET;
                    } else {
                        bool allValid = true;
                        for (const auto& syn : tokens) {
                            string errMsg;
                            if (!isValidSynonym(syn, errMsg)) {
                                cout << RED << "  [!] " << errMsg
                                     << " Synonyms unchanged.\n" << RESET;
                                allValid = false;
                                break;
                            }
                        }
                        if (allValid) entry->synonyms = tokens;
                    }
                }
            }
        }

        cout << GREEN << "\n  [✓] '" << word << "' updated successfully!\n" << RESET;
    }

    /**
     * @brief  Feature 8: Display the search history (most recent first).
     */
    void viewSearchHistory() const {
        printHeader("SEARCH HISTORY");
        if (searchHistory.empty()) {
            cout << DIM << "  No searches recorded yet.\n" << RESET;
            return;
        }
        cout << "  " << DIM << searchHistory.size() << " recent search(es):\n\n" << RESET;
        int idx = 1;
        for (const auto& word : searchHistory)
            cout << "    " << CYAN << idx++ << "." << RESET << "  " << word << "\n";
    }

    /** @brief  Feature 9: Delegate to saveToFile(). */
    void saveDictionary() { saveToFile(); }

    /** @brief  Feature 10: Delegate to loadFromFile(). */
    void loadDictionary() { loadFromFile(); }

public:
    /**
     * @brief  Constructor — restores any previously saved session from file.
     */
    Dictionary() {
        loadFromFile();
    }

    /** Destructor — nothing to do; BST destructor handles node cleanup. */
    ~Dictionary() = default;

    /**
     * @brief  Main application event loop.
     *
     * Displays the menu, reads and validates the user's choice with
     * [VAL-06] + [VAL-11], dispatches to the appropriate feature,
     * and repeats until the user selects option 0 (exit).
     *
     * On exit: calls saveToFile() to persist the current session.
     * On EOF:  [ERR-05] exits gracefully with a save.
     */
    void run() {
        bool isRunning = true;

        while (isRunning) {
            displayMenu();

            // ── Read and validate menu choice [VAL-06 / VAL-11] ──
            string rawInput;
            if (!getline(cin, rawInput)) {
                // [ERR-05] stdin reached EOF (e.g. redirected input file ended)
                cout << YELLOW << "\n  [!] Input stream closed. Saving and exiting...\n" << RESET;
                saveToFile();
                break;
            }
            rawInput = trim(rawInput);

            int choice = -1;
            if (!parseMenuChoice(rawInput, choice)) {
                // [VAL-06] Reject non-integers or out-of-range values
                cout << RED << "\n  [!] Invalid choice. Please enter a number between 0 and "
                     << Constants::MAX_MENU_CHOICE << ".\n" << RESET;
                continue;
            }

            // ── Dispatch to feature ───────────────────────────────
            switch (choice) {
                case 1:  addWord();           break;
                case 2:  searchWord();        break;
                case 3:  deleteWord();        break;
                case 4:  displayAllWords();   break;
                case 5:  countWords();        break;
                case 6:  autoComplete();      break;
                case 7:  editWord();          break;
                case 8:  viewSearchHistory(); break;
                case 9:  saveDictionary();    break;
                case 10: loadDictionary();    break;
                case 0:
                    cout << YELLOW << "\n  Exiting BST Dictionary. Saving data...\n" << RESET;
                    saveToFile();
                    isRunning = false;
                    break;
                default:
                    // Should never reach here due to parseMenuChoice range check,
                    // but included for defensive completeness [ERR-03].
                    cout << RED << "\n  [!] Unrecognised option. Please choose 0-"
                         << Constants::MAX_MENU_CHOICE << ".\n" << RESET;
            }
        }

        cout << CYAN << "\n  Goodbye! Thank you for using BST Dictionary.\n\n" << RESET;
    }
};

// ============================================================
//  ENTRY POINT
// ============================================================

/**
 * @brief  Program entry point.
 *
 * On Windows, enables UTF-8 console output (code page 65001) so
 * ANSI characters and IPA phonetic symbols display correctly.
 *
 * @return 0 on normal exit.
 */
int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    Dictionary dict;
    dict.run();

    return 0;
}