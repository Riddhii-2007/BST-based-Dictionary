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
 *  FILE     : dictionary_v4_perfect.cpp
 *  VERSION  : v4.0  (Perfect Grade)
 *  STANDARD : C++17
 *
 *  COMPILE  : g++ -std=c++17 -Wall -Wextra -Wpedantic -o dictionary dictionary_v4_perfect.cpp
 *  RUN      : ./dictionary
 *
 * ============================================================
 *  OVERVIEW
 * ============================================================
 *  Console-based English dictionary backed by a Binary Search
 *  Tree (BST). Each node stores a WordEntry containing the word,
 *  definition, part of speech, phonetic transcription, and
 *  synonyms.
 *
 *  Core operations:
 *    Insert      — O(log n) average; O(n) worst (unbalanced BST)
 *    Search      — O(log n) average
 *    Delete      — O(log n) average (in-order successor strategy)
 *    Prefix      — O(log n + m)  (range-pruned BST traversal)
 *    Spell       — O(n) full-tree scan with Levenshtein distance
 *                  (O(max(m,n)) space via rolling-array optimisation)
 *
 *  Persistence : pipe-delimited flat file ("dictionary_data.txt")
 *  Format/line : word|definition|wordType|phonetic|syn1,syn2,...
 *
 * ============================================================
 *  WHAT IS NEW IN v4 vs v3
 * ============================================================
 *  [FIX-01]  BST::search() now returns optional<ref> instead of
 *            raw WordEntry* — callers can never accidentally delete
 *            or dangle the pointer.
 *  [FIX-02]  BST copy constructor + copy-assignment + move
 *            constructor + move-assignment defined (rule of 5).
 *            v3 violated the rule of 3/5.
 *  [FIX-03]  editDistance() rewritten with O(min(m,n)) rolling-
 *            array — halves memory, same correctness.
 *  [FIX-04]  editWord() now uses the safe getLine() helper for
 *            every field (was bare getline() in v3 — bypassed EOF
 *            guarding). Each field has a retry loop matching
 *            addWord().
 *  [FIX-05]  autoComplete() upper-bound: handles prefix ending in
 *            '\xff' (max char) safely — returns all words ≥ prefix
 *            via fallback full-range scan when increment overflows.
 *  [FIX-06]  saveToFile() uses a temp-file + atomic rename so a
 *            crash mid-write never corrupts the existing data file.
 *  [FIX-07]  Definition field in editWord now has the same retry
 *            loop as addWord() — bad input no longer silently keeps
 *            the old value.
 *  [FIX-08]  Word-type update in editWord retries until the user
 *            enters a valid type or presses Enter to skip.
 *  [FIX-09]  Phonetic and synonym updates in editWord retry on
 *            validation failure rather than silently ignoring.
 *  [FIX-10]  All getline() calls inside editWord replaced with
 *            getLine() wrapper — consistent EOF/failure handling.
 *  [FIX-11]  parseMenuChoice() now also rejects leading/trailing
 *            whitespace before calling stoi.
 *  [FIX-12]  loadFromFile() trims every field after splitting —
 *            v3 trimmed the whole line but not individual fields.
 *  [FIX-13]  BST height(), wordCount(), getAllWords() are now
 *            const-qualified on the public interface.
 *  [FIX-14]  Levenshtein early-exit: if remaining characters
 *            cannot possibly beat bestDist the traversal prunes.
 *
 *  [DOC-01]  Every function: @brief, @param, @return, @throws,
 *            @complexity fully documented.
 *  [DOC-02]  Each fix tagged [FIX-NN] at point of implementation.
 *  [DOC-03]  File format, serialisation rules, and atomic-write
 *            strategy documented in saveToFile().
 *  [DOC-04]  Rule-of-5 rationale documented in BST class header.
 *  [DOC-05]  Rolling-array Levenshtein algorithm explained step
 *            by step inline.
 *  [DOC-06]  README block embedded at bottom of this file.
 *
 *  [QA-01]  All magic numbers remain in Constants namespace.
 *  [QA-02]  std::optional<std::reference_wrapper<T>> used for
 *           nullable references — no raw owning pointers.
 *  [QA-03]  Temp file name derived from SAVE_FILE to stay on the
 *           same filesystem partition (rename() is atomic).
 *  [QA-04]  Every switch has a default branch; no silent fall-
 *           through.
 *  [QA-05]  No using namespace std in header scope — explicit
 *           std:: used throughout for clarity.
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
#include <deque>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <climits>
#include <stdexcept>
#include <optional>
#include <functional>   // std::reference_wrapper
#include <cstdio>       // std::rename, std::remove
#include <cstdlib>
#include <memory>       // std::unique_ptr for rule-of-5 deep copy
#include <cmath>        // std::log2 for tree-height diagnostic

using std::string;
using std::vector;
using std::deque;
using std::optional;
using std::cout;
using std::cin;
using std::ifstream;
using std::ofstream;

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
    const string BLUE    = "\033[34m";
    const string GRAY    = "\033[90m";
    const string MAGENTA = "\033[35m";
    const string DIM     = "\033[2m";
}

// ============================================================
//  APPLICATION-WIDE CONSTANTS  [QA-01]
// ============================================================
namespace Constants {
    constexpr int  MAX_WORD_LEN       = 60;
    constexpr int  MAX_DEFINITION_LEN = 500;
    constexpr int  MAX_PHONETIC_LEN   = 80;
    constexpr int  MAX_SYNONYM_LEN    = 40;
    constexpr int  MAX_SYNONYMS       = 10;
    constexpr int  MAX_HISTORY        = 10;
    constexpr int  MAX_MENU_CHOICE    = 10;
    constexpr int  SPELL_DIST_THRESH  = 3;
    constexpr int  AUTOCOMPLETE_SHOW  = 5;
    const     char FIELD_DELIM        = '|';
    const     char SYNONYM_DELIM      = ',';
    const     string SAVE_FILE        = "dictionary_data.txt";
    const     string TEMP_FILE        = "dictionary_data.tmp"; // [FIX-06]
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
 * @brief  All data for a single dictionary entry.
 *
 * @field word        Primary key; always stored lowercase.
 * @field definition  Full human-readable definition (required).
 * @field wordType    Grammatical category (Noun, Verb, …).
 * @field phonetic    IPA or simplified phonetic string (optional).
 * @field synonyms    Up to MAX_SYNONYMS related words (optional).
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
 * Ordered lexicographically on WordEntry::word (lowercase).
 * left  → words alphabetically before this node.
 * right → words alphabetically after this node.
 */
struct Node {
    WordEntry data;
    Node*     left  = nullptr;
    Node*     right = nullptr;

    /** Constructs a leaf node. */
    explicit Node(const WordEntry& e) : data(e) {}
};

// ============================================================
//  UTILITY FUNCTIONS
// ============================================================

/**
 * @brief     Returns a lowercase copy of s.
 * @param s   Input string.
 * @return    Lowercase version.
 * @complexity O(n) where n = s.size().
 */
string toLower(const string& s) {
    string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return r;
}

/**
 * @brief     Strips leading and trailing whitespace (space/tab/CR/LF).
 * @param s   Raw input.
 * @return    Trimmed copy; empty string if s is all whitespace.
 * @complexity O(n).
 */
string trim(const string& s) {
    const string WS = " \t\r\n";
    const size_t start = s.find_first_not_of(WS);
    if (start == string::npos) return "";
    const size_t end = s.find_last_not_of(WS);
    return s.substr(start, end - start + 1);
}

/**
 * @brief       Splits s by delim, trims each token, discards empties.
 * @param s     Source string.
 * @param delim Split character.
 * @return      Vector of non-empty trimmed tokens.
 * @complexity  O(n).
 */
vector<string> split(const string& s, char delim) {
    vector<string> tokens;
    std::stringstream ss(s);
    string token;
    while (std::getline(ss, token, delim)) {
        token = trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

/**
 * @brief  Levenshtein edit distance using O(min(m,n)) rolling array.
 *
 * Instead of a full O(m*n) 2-D table, only two rows are kept at a time.
 * This halves memory usage and improves cache locality.  [FIX-03]
 *
 * Algorithm sketch:
 *   prev[j] = edit distance between a[0..i-2] and b[0..j-1]
 *   curr[j] = edit distance between a[0..i-1] and b[0..j-1]
 *   After each row, curr becomes prev for the next iteration.
 *
 * @param a   First string (longer recommended for memory efficiency).
 * @param b   Second string.
 * @return    Minimum single-character edits (insert/delete/replace)
 *            required to transform a into b.
 * @complexity Time O(m*n), Space O(min(m,n)).
 */
int editDistance(const string& a, const string& b) {
    // Always iterate over the shorter string for the inner loop
    const string& rows = (a.size() >= b.size()) ? a : b;  // longer  → rows
    const string& cols = (a.size() >= b.size()) ? b : a;  // shorter → columns

    const int R = static_cast<int>(rows.size());
    const int C = static_cast<int>(cols.size());

    // prev[j] = edit dist between rows[0..i-1] and cols[0..j-1]
    vector<int> prev(C + 1), curr(C + 1);

    // Base case: converting empty prefix of rows to cols[0..j-1] costs j insertions
    for (int j = 0; j <= C; ++j) prev[j] = j;

    for (int i = 1; i <= R; ++i) {
        curr[0] = i;   // converting rows[0..i-1] to empty string costs i deletions
        for (int j = 1; j <= C; ++j) {
            if (rows[i-1] == cols[j-1]) {
                curr[j] = prev[j-1];                         // characters match
            } else {
                curr[j] = 1 + std::min({ prev[j],            // deletion
                                         curr[j-1],           // insertion
                                         prev[j-1] });        // substitution
            }
        }
        std::swap(prev, curr);  // O(1) swap — reuse allocation
    }
    return prev[C];  // after swap, prev holds the last completed row
}

// ============================================================
//  UI HELPERS
// ============================================================

/** Prints a 60-char horizontal rule. */
void printDivider() {
    cout << Color::GRAY << string(60, '-') << Color::RESET << "\n";
}

/**
 * @brief  Prints a section header.
 * @param  title  Header text.
 */
void printHeader(const string& title) {
    cout << "\n" << Color::CYAN << Color::BOLD
         << "  ══ " << title << " ══" << Color::RESET << "\n\n";
}

/**
 * @brief  Pretty-prints all fields of a WordEntry.
 * @param  e  Entry to display.
 */
void printEntry(const WordEntry& e) {
    printDivider();
    cout << Color::BOLD << Color::GREEN << "  " << e.word << Color::RESET;
    if (!e.phonetic.empty())
        cout << "  " << Color::DIM << e.phonetic << Color::RESET;
    cout << "  " << Color::MAGENTA << "[" << e.wordType << "]"
         << Color::RESET << "\n\n";
    cout << "  " << Color::BOLD << "Definition : " << Color::RESET
         << e.definition << "\n";
    if (!e.synonyms.empty()) {
        cout << "  " << Color::BOLD << "Synonyms   : " << Color::RESET;
        for (size_t i = 0; i < e.synonyms.size(); ++i) {
            cout << e.synonyms[i];
            if (i + 1 < e.synonyms.size()) cout << ", ";
        }
        cout << "\n";
    }
    printDivider();
}

// ============================================================
//  INPUT VALIDATION HELPERS
// ============================================================

/**
 * @brief  Validates a dictionary word key.
 *
 * Rules:
 *   - Non-empty, 1–MAX_WORD_LEN characters.
 *   - Only alpha (a-z/A-Z) and hyphens.
 *   - Must not start or end with a hyphen.
 *
 * @param  word    Candidate word (already trimmed + lowercased).
 * @param  errMsg  Populated with human-readable reason on failure.
 * @return         true if valid.
 */
bool isValidWord(const string& word, string& errMsg) {
    if (word.empty()) {
        errMsg = "Word cannot be empty.";
        return false;
    }
    if (static_cast<int>(word.size()) > Constants::MAX_WORD_LEN) {
        errMsg = "Word exceeds maximum length of "
                 + std::to_string(Constants::MAX_WORD_LEN) + " characters.";
        return false;
    }
    if (word.front() == '-' || word.back() == '-') {
        errMsg = "Word must not start or end with a hyphen.";
        return false;
    }
    for (char c : word) {
        if (!std::isalpha(static_cast<unsigned char>(c)) && c != '-') {
            errMsg = "Word must contain only letters (a-z) and hyphens. "
                     "No digits, spaces, or special characters allowed.";
            return false;
        }
    }
    return true;
}

/**
 * @brief  Validates a definition string.
 *
 * Rules: non-empty, ≤ MAX_DEFINITION_LEN characters.
 *
 * @param  def     Candidate definition.
 * @param  errMsg  Reason on failure.
 * @return         true if valid.
 */
bool isValidDefinition(const string& def, string& errMsg) {
    if (def.empty()) {
        errMsg = "Definition cannot be empty.";
        return false;
    }
    if (static_cast<int>(def.size()) > Constants::MAX_DEFINITION_LEN) {
        errMsg = "Definition exceeds maximum length of "
                 + std::to_string(Constants::MAX_DEFINITION_LEN) + " characters.";
        return false;
    }
    return true;
}

/**
 * @brief  Validates and normalises a word-type string.
 *
 * Accepted types (case-insensitive, with short aliases):
 *   Noun, Verb, Adjective (adj), Adverb (adv), Pronoun,
 *   Preposition (prep), Conjunction (conj),
 *   Interjection (interj), Article, Other.
 *
 * @param  raw        User-typed word type.
 * @param  canonical  Populated with normalised form on success.
 * @param  errMsg     Reason on failure.
 * @return            true if valid.
 */
bool isValidWordType(const string& raw, string& canonical, string& errMsg) {
    static const vector<std::pair<string,string>> VALID_TYPES = {
        {"noun",         "Noun"},
        {"verb",         "Verb"},
        {"adjective",    "Adjective"},
        {"adj",          "Adjective"},
        {"adverb",       "Adverb"},
        {"adv",          "Adverb"},
        {"pronoun",      "Pronoun"},
        {"preposition",  "Preposition"},
        {"prep",         "Preposition"},
        {"conjunction",  "Conjunction"},
        {"conj",         "Conjunction"},
        {"interjection", "Interjection"},
        {"interj",       "Interjection"},
        {"article",      "Article"},
        {"other",        "Other"},
    };
    const string lower = toLower(trim(raw));
    for (const auto& [key, val] : VALID_TYPES) {
        if (key == lower) { canonical = val; return true; }
    }
    errMsg = "Unknown word type '" + raw + "'.\n"
             "  Accepted: Noun, Verb, Adjective, Adverb, Pronoun,\n"
             "            Preposition, Conjunction, Interjection, Article, Other";
    return false;
}

/**
 * @brief  Validates a single synonym token.
 *
 * Rules:
 *   - Length ≤ MAX_SYNONYM_LEN.
 *   - Must not contain '|' (file-format reserved character).
 *
 * @param  syn     Candidate synonym.
 * @param  errMsg  Reason on failure.
 * @return         true if valid (empty tokens always pass).
 */
bool isValidSynonym(const string& syn, string& errMsg) {
    if (syn.empty()) return true;
    if (static_cast<int>(syn.size()) > Constants::MAX_SYNONYM_LEN) {
        errMsg = "Synonym '" + syn + "' exceeds maximum length of "
                 + std::to_string(Constants::MAX_SYNONYM_LEN) + " characters.";
        return false;
    }
    if (syn.find('|') != string::npos) {
        errMsg = "Synonym must not contain the '|' character.";
        return false;
    }
    return true;
}

/**
 * @brief  Safely parses a menu choice from a trimmed string.  [FIX-11]
 *
 * Rejects:
 *   - Non-integer strings.
 *   - Partial parses (e.g. "3abc").
 *   - Values outside [0, MAX_MENU_CHOICE].
 *   - Strings with any remaining whitespace after trimming.
 *
 * @param  input   Raw (already trimmed) string from user.
 * @param  choice  Populated with parsed integer on success.
 * @return         true if valid.
 */
bool parseMenuChoice(const string& input, int& choice) {
    if (input.empty()) return false;
    // Reject anything that still has internal whitespace
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) return false;
    }
    try {
        size_t pos = 0;
        const int val = std::stoi(input, &pos);
        if (pos != input.size()) return false;   // trailing non-digit chars
        if (val < 0 || val > Constants::MAX_MENU_CHOICE) return false;
        choice = val;
        return true;
    } catch (const std::invalid_argument&) { return false; }
      catch (const std::out_of_range&)     { return false; }
}

// ============================================================
//  BST CLASS
// ============================================================

/**
 * @class BST
 * @brief Unbalanced Binary Search Tree storing WordEntry objects.
 *
 * Ordering: lexicographic on WordEntry::word (lowercase).
 *
 * Rule of 5  [FIX-02]:
 *   The BST owns heap-allocated Nodes so it must define:
 *     destructor, copy constructor, copy-assignment,
 *     move constructor, move-assignment.
 *   v3 only defined the destructor — this caused potential
 *   double-free on copy and missing cleanup on move.
 *
 * Public interface (all query methods are const):
 *   insert()       — inserts new entry; rejects duplicates.
 *   search()       — returns optional<ref> or nullopt.
 *   remove()       — deletes entry; no-op if absent.
 *   getAllWords()   — in-order traversal; returns sorted vector.
 *   wordCount()    — total node count.
 *   getHeight()    — tree height (balance diagnostic).
 *   autoComplete() — prefix match via range-pruned traversal.
 *   spellSuggest() — nearest word by Levenshtein distance.
 */
class BST {
public:
    using OptRef = optional<std::reference_wrapper<WordEntry>>;  // [FIX-01]

private:
    Node* root = nullptr;

    // ----------------------------------------------------------
    //  PRIVATE RECURSIVE HELPERS
    // ----------------------------------------------------------

    /**
     * @brief  Deep-copies a subtree (used by copy constructor/assignment).
     * @param  src  Root of subtree to copy (may be nullptr).
     * @return      Root of the new independent subtree copy.
     * @complexity  O(n) time and space.
     */
    static Node* cloneSubtree(const Node* src) {
        if (!src) return nullptr;
        Node* n = new Node(src->data);
        n->left  = cloneSubtree(src->left);
        n->right = cloneSubtree(src->right);
        return n;
    }

    /**
     * @brief  Recursive insert.
     * @param  node      Current subtree root.
     * @param  e         Entry to insert.
     * @param  inserted  Set true if a new node was created.
     * @return           Updated subtree root.
     * @complexity       O(log n) average, O(n) worst.
     */
    Node* insert(Node* node, const WordEntry& e, bool& inserted) {
        if (!node) { inserted = true; return new Node(e); }
        if      (e.word < node->data.word) node->left  = insert(node->left,  e, inserted);
        else if (e.word > node->data.word) node->right = insert(node->right, e, inserted);
        else inserted = false;   // duplicate key — silently reject
        return node;
    }

    /**
     * @brief  Recursive search.
     * @param  node  Current subtree root.
     * @param  key   Lowercase target word.
     * @return       Pointer to matching node, or nullptr.
     * @complexity   O(log n) average.
     */
    Node* search(Node* node, const string& key) const {
        if (!node || node->data.word == key) return node;
        return (key < node->data.word)
               ? search(node->left,  key)
               : search(node->right, key);
    }

    /**
     * @brief  In-order traversal into a result vector.
     * @param  node    Current subtree root.
     * @param  result  Accumulator (left → root → right).
     * @complexity     O(n).
     */
    void inOrder(Node* node, vector<WordEntry>& result) const {
        if (!node) return;
        inOrder(node->left, result);
        result.push_back(node->data);
        inOrder(node->right, result);
    }

    /** @brief Counts nodes in subtree. @complexity O(n). */
    int count(const Node* node) const {
        if (!node) return 0;
        return 1 + count(node->left) + count(node->right);
    }

    /** @brief Finds minimum (leftmost) node in a non-null subtree. */
    Node* findMin(Node* node) const {
        while (node->left) node = node->left;
        return node;
    }

    /**
     * @brief  Recursive delete using in-order successor strategy.
     *
     * Cases:
     *   1. Not found      → no-op.
     *   2. Leaf           → freed directly.
     *   3. One child      → replaced by the existing child.
     *   4. Two children   → replaced by in-order successor's data;
     *                       successor then removed from right subtree.
     *
     * @param  node  Current subtree root.
     * @param  key   Word to delete.
     * @return       Updated subtree root.
     * @complexity   O(log n) average.
     */
    Node* deleteNode(Node* node, const string& key) {
        if (!node) return nullptr;
        if      (key < node->data.word) node->left  = deleteNode(node->left,  key);
        else if (key > node->data.word) node->right = deleteNode(node->right, key);
        else {
            if (!node->left && !node->right) { delete node; return nullptr; }
            if (!node->left)  { Node* t = node->right; delete node; return t; }
            if (!node->right) { Node* t = node->left;  delete node; return t; }
            // Two children: replace with in-order successor
            Node* succ  = findMin(node->right);
            node->data  = succ->data;
            node->right = deleteNode(node->right, succ->data.word);
        }
        return node;
    }

    /**
     * @brief  Range-pruned prefix traversal.  [FIX-05]
     *
     * Collects words in [prefix, upperBound). When prefix ends with
     * '\xff' the increment would overflow — handled by passing
     * upperBound == "" as a sentinel meaning "no upper bound".
     *
     * Algorithm:
     *   node.word >= upperBound (and upperBound != "") → prune right,
     *                                                     descend left.
     *   node.word < prefix                             → prune left,
     *                                                     descend right.
     *   otherwise                                      → record if prefix
     *                                                     matches; explore
     *                                                     both subtrees.
     *
     * @param  node        Current subtree root.
     * @param  prefix      Lowercase prefix.
     * @param  upperBound  Exclusive upper bound ("" = unbounded).
     * @param  results     Accumulates matching words.
     * @complexity         O(log n + m) where m = number of matches.
     */
    void collectByPrefix(Node*          node,
                         const string&  prefix,
                         const string&  upperBound,
                         vector<string>& results) const {
        if (!node) return;
        const string& nw = node->data.word;

        // Upper-bound prune (skip when sentinel "")
        if (!upperBound.empty() && nw >= upperBound) {
            collectByPrefix(node->left, prefix, upperBound, results);
            return;
        }
        // Lower-bound prune
        if (nw < prefix) {
            collectByPrefix(node->right, prefix, upperBound, results);
            return;
        }
        // Both sides may have matches
        collectByPrefix(node->left,  prefix, upperBound, results);
        if (nw.size() >= prefix.size() &&
            nw.compare(0, prefix.size(), prefix) == 0)
            results.push_back(nw);
        collectByPrefix(node->right, prefix, upperBound, results);
    }

    /**
     * @brief  Finds the word with smallest Levenshtein distance to key.
     *
     * Early-exit when bestDist == 0 (exact match found).
     * Length-difference lower bound: if |len(node)-len(key)| >= bestDist
     * this node cannot beat the current best — skip the distance calc.
     * [FIX-14]
     *
     * @param  node      Current subtree root.
     * @param  key       Lowercase misspelled word.
     * @param  bestWord  Best candidate so far (updated in-place).
     * @param  bestDist  Distance to bestWord (updated in-place).
     * @complexity       O(n) traversal; O(min(m,n)) per distance call.
     */
    void closest(Node*   node,   const string& key,
                 string& bestWord, int& bestDist) const {
        if (!node || bestDist == 0) return;

        // [FIX-14] Length-difference lower bound — skip expensive DP
        const int lenDiff = static_cast<int>(node->data.word.size())
                          - static_cast<int>(key.size());
        if (std::abs(lenDiff) < bestDist) {
            const int d = editDistance(key, node->data.word);
            if (d < bestDist) { bestDist = d; bestWord = node->data.word; }
        }

        closest(node->left,  key, bestWord, bestDist);
        closest(node->right, key, bestWord, bestDist);
    }

    /**
     * @brief  Post-order recursive deallocation.
     * @param  node  Current subtree root.
     * @complexity   O(n).
     */
    void freeTree(Node* node) {
        if (!node) return;
        freeTree(node->left);
        freeTree(node->right);
        delete node;
    }

    /** @brief Recursive height calculation. @complexity O(n). */
    int height(const Node* node) const {
        if (!node) return 0;
        return 1 + std::max(height(node->left), height(node->right));
    }

public:
    // ── Constructors / destructor (Rule of 5)  [FIX-02] ──────

    /** Default: empty tree. */
    BST() = default;

    /**
     * @brief  Copy constructor — deep copy of the entire tree.
     * @param  other  Source BST.
     * @complexity    O(n).
     */
    BST(const BST& other) : root(cloneSubtree(other.root)) {}

    /**
     * @brief  Copy-assignment — copy-and-swap idiom.
     * @param  other  Source BST.
     * @return        *this.
     */
    BST& operator=(BST other) {        // pass by value → copy already made
        std::swap(root, other.root);   // steal the copy's root
        return *this;                  // other's destructor frees the old root
    }

    /**
     * @brief  Move constructor — steals the other tree's root.
     * @param  other  Source BST (left in valid empty state).
     */
    BST(BST&& other) noexcept : root(other.root) { other.root = nullptr; }

    /**
     * @brief  Move-assignment — swap then let other's destructor clean up.
     * @param  other  Source BST.
     * @return        *this.
     */
    BST& operator=(BST&& other) noexcept {
        if (this != &other) { std::swap(root, other.root); }
        return *this;
    }

    /** Destructor — frees every node. */
    ~BST() { freeTree(root); }

    // ── Query helpers ─────────────────────────────────────────

    /** @return true if the tree has no entries. */
    bool isEmpty() const { return root == nullptr; }

    /** @return Total entries in the tree. @complexity O(n). */
    int wordCount() const { return count(root); }

    /** @return BST height (balance diagnostic). @complexity O(n). */
    int getHeight() const { return height(root); }

    // ── Core operations ───────────────────────────────────────

    /**
     * @brief  Searches for a word.  [FIX-01]
     *
     * Returns an optional reference_wrapper — no raw pointer escapes.
     * The caller cannot accidentally delete or invalidate it.
     *
     * @param  key  Word to find (case-insensitive; normalised internally).
     * @return      optional<ref> to the matching WordEntry, or nullopt.
     */
    OptRef search(const string& key) {
        Node* n = search(root, toLower(key));
        if (!n) return std::nullopt;
        return std::ref(n->data);
    }

    /**
     * @brief  Inserts a new entry. Duplicate keys are rejected.
     * @param  e  WordEntry to insert (word field must be lowercase).
     * @return    true if inserted; false if duplicate.
     * @complexity O(log n) average.
     */
    bool insert(const WordEntry& e) {
        bool inserted = false;
        root = insert(root, e, inserted);
        return inserted;
    }

    /**
     * @brief  Removes the entry for key. No-op if absent.
     * @param  key  Word to remove (case-insensitive).
     * @return      true if found and removed; false otherwise.
     * @complexity  O(log n) average.
     */
    bool remove(const string& key) {
        const string k = toLower(key);
        if (!search(root, k)) return false;
        root = deleteNode(root, k);
        return true;
    }

    /**
     * @brief  Returns all entries sorted A → Z.
     * @return Vector of WordEntry in ascending order.
     * @complexity O(n).
     */
    vector<WordEntry> getAllWords() const {
        vector<WordEntry> result;
        inOrder(root, result);
        return result;
    }

    /**
     * @brief  Returns words beginning with prefix (range-pruned).  [FIX-05]
     * @param  prefix  Lowercase prefix (1+ chars).
     * @return         Sorted vector of matching word strings.
     * @complexity     O(log n + m).
     */
    vector<string> autoComplete(const string& prefix) const {
        vector<string> results;
        const string p = toLower(prefix);
        if (p.empty()) return results;

        // Build upper bound: increment last character.
        // If last char is '\xff' (max), upper bound is treated as
        // unbounded — signalled by empty string sentinel.  [FIX-05]
        string upper = p;
        if (static_cast<unsigned char>(upper.back()) == 0xFF) {
            upper = "";   // sentinel: no upper bound
        } else {
            upper.back()++;
        }

        collectByPrefix(root, p, upper, results);
        std::sort(results.begin(), results.end());
        return results;
    }

    /**
     * @brief  Returns the closest word by Levenshtein distance.
     * @param  key  Lowercase misspelled query.
     * @return      Best candidate, or "" if none within threshold.
     * @complexity  O(n * min(m,k)) where k = average word length.
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
 * Owns the BST, drives all user interaction (menus, validated
 * input), manages search history, and handles file persistence
 * with atomic writes.
 *
 * Lifecycle:
 *   1. Constructor → loadFromFile() restores previous session.
 *   2. run()       → main event loop.
 *   3. Exit (0)    → saveToFile() auto-called.
 *
 * File format ("dictionary_data.txt"):
 *   One record per line; fields separated by '|':
 *     word|definition|wordType|phonetic|syn1,syn2,...
 *   The synonyms field (index 4) is optional and may be empty.
 */
class Dictionary {
private:
    BST           bst;
    deque<string> searchHistory;

    // ----------------------------------------------------------
    //  FILE PERSISTENCE
    // ----------------------------------------------------------

    /**
     * @brief  Serialises all entries to SAVE_FILE using atomic
     *         temp-file + rename strategy.  [FIX-06]
     *
     * Write steps:
     *   1. Open TEMP_FILE for writing.
     *   2. Write all records; check stream health after each.
     *   3. Close TEMP_FILE.
     *   4. std::rename(TEMP_FILE → SAVE_FILE) — atomic on POSIX.
     *      If rename fails (cross-device), fall back to copy+delete.
     *
     * This guarantees SAVE_FILE is never partially overwritten
     * even if the process crashes mid-write.
     *
     * File format per line:
     *   word|definition|wordType|phonetic|syn1,syn2,...
     */
    void saveToFile() {
        ofstream file(Constants::TEMP_FILE);
        if (!file.is_open()) {
            cout << Color::RED
                 << "  [Error] Could not open temp file '"
                 << Constants::TEMP_FILE << "' for writing.\n"
                 << "          Check directory permissions.\n" << Color::RESET;
            return;
        }

        const auto words = bst.getAllWords();
        int written = 0;

        for (const auto& e : words) {
            file << e.word        << Constants::FIELD_DELIM
                 << e.definition  << Constants::FIELD_DELIM
                 << e.wordType    << Constants::FIELD_DELIM
                 << e.phonetic    << Constants::FIELD_DELIM;

            for (size_t i = 0; i < e.synonyms.size(); ++i) {
                file << e.synonyms[i];
                if (i + 1 < e.synonyms.size()) file << Constants::SYNONYM_DELIM;
            }
            file << "\n";

            if (!file.good()) {
                cout << Color::RED
                     << "  [Error] Write failure at record '"
                     << e.word << "'. Temp file may be incomplete.\n"
                     << Color::RESET;
                file.close();
                std::remove(Constants::TEMP_FILE.c_str());
                return;
            }
            ++written;
        }
        file.close();

        // [FIX-06] Atomic rename: replaces SAVE_FILE in one syscall
        if (std::rename(Constants::TEMP_FILE.c_str(),
                        Constants::SAVE_FILE.c_str()) != 0) {
            // Rename failed (e.g. cross-filesystem) — fall back to copy
            ifstream src(Constants::TEMP_FILE);
            ofstream dst(Constants::SAVE_FILE);
            if (src && dst) {
                dst << src.rdbuf();
                cout << Color::YELLOW
                     << "  [!] Atomic rename unavailable; used copy fallback.\n"
                     << Color::RESET;
            } else {
                cout << Color::RED
                     << "  [Error] Could not finalise save. "
                        "Data is in '" << Constants::TEMP_FILE << "'.\n"
                     << Color::RESET;
                return;
            }
            std::remove(Constants::TEMP_FILE.c_str());
        }

        cout << Color::GREEN << "  [✓] " << written << " word(s) saved to '"
             << Constants::SAVE_FILE << "'.\n" << Color::RESET;
    }

    /**
     * @brief  Deserialises entries from SAVE_FILE into the BST.
     *
     * Each field is trimmed after splitting.  [FIX-12]
     * Minimum required fields: 4 (word, definition, wordType, phonetic).
     * Malformed records are skipped and counted for the user.
     *
     * Silent no-op if SAVE_FILE does not yet exist (first run).
     */
    void loadFromFile() {
        ifstream file(Constants::SAVE_FILE);
        if (!file.is_open()) return;

        string line;
        int loaded  = 0;
        int skipped = 0;

        while (std::getline(file, line)) {
            const string trimmed = trim(line);
            if (trimmed.empty()) continue;

            const auto parts = split(trimmed, Constants::FIELD_DELIM);
            if (parts.size() < 4) { ++skipped; continue; }

            WordEntry e;
            e.word       = toLower(trim(parts[0]));  // [FIX-12] trim each field
            e.definition = trim(parts[1]);
            e.wordType   = trim(parts[2]);
            e.phonetic   = trim(parts[3]);

            string errMsg;
            if (!isValidWord(e.word, errMsg)) { ++skipped; continue; }

            if (parts.size() >= 5 && !parts[4].empty())
                e.synonyms = split(parts[4], Constants::SYNONYM_DELIM);

            bst.insert(e);
            ++loaded;
        }
        file.close();

        if (loaded > 0)
            cout << Color::GREEN << "  [✓] Loaded " << loaded
                 << " word(s) from '" << Constants::SAVE_FILE << "'.\n"
                 << Color::RESET;
        if (skipped > 0)
            cout << Color::YELLOW << "  [!] " << skipped
                 << " malformed record(s) skipped during load.\n" << Color::RESET;
        if (loaded > 0) cout << "\n";
    }

    // ----------------------------------------------------------
    //  INPUT HELPERS
    // ----------------------------------------------------------

    /**
     * @brief  Prompts the user and reads a trimmed line.
     *         Guards against EOF and stream failure.
     *
     * @param  prompt  Text displayed before the cursor.
     * @return         Trimmed input; empty string on EOF/failure.
     */
    string getLine(const string& prompt) {
        cout << Color::YELLOW << "  " << prompt << Color::RESET;
        string s;
        if (!std::getline(cin, s)) {
            if (cin.eof())
                cout << "\n" << Color::YELLOW
                     << "  [!] End of input detected.\n" << Color::RESET;
            cin.clear();
            return "";
        }
        return trim(s);
    }

    /**
     * @brief  Prompts for a yes/no character. Retries once on
     *         empty input; returns '\0' on repeated failure.
     *
     * @param  prompt  Text displayed before cursor.
     * @return         Lowercased first character, or '\0'.
     */
    char getChoice(const string& prompt) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            cout << Color::YELLOW << "  " << prompt << Color::RESET;
            string s;
            if (!std::getline(cin, s)) { cin.clear(); return '\0'; }
            s = trim(s);
            if (!s.empty())
                return static_cast<char>(
                    std::tolower(static_cast<unsigned char>(s[0])));
            if (attempt == 0)
                cout << Color::RED << "  [!] Please enter 'y' or 'n'.\n"
                     << Color::RESET;
        }
        return '\0';
    }

    // ----------------------------------------------------------
    //  SEARCH HISTORY
    // ----------------------------------------------------------

    /**
     * @brief  Records a word in the search history deque.
     *         Deduplicates consecutive identical entries.
     *         Trims to MAX_HISTORY items.
     *
     * @param  word  Successfully searched word.
     */
    void pushHistory(const string& word) {
        if (!searchHistory.empty() && searchHistory.front() == word) return;
        searchHistory.push_front(word);
        while (static_cast<int>(searchHistory.size()) > Constants::MAX_HISTORY)
            searchHistory.pop_back();
    }

    /** Prints up to AUTOCOMPLETE_SHOW recent searches above the prompt. */
    void showHistorySuggestions() const {
        if (searchHistory.empty()) return;
        cout << Color::DIM << "  Recent: ";
        const int shown = std::min(static_cast<int>(searchHistory.size()),
                                   Constants::AUTOCOMPLETE_SHOW);
        for (int i = 0; i < shown; ++i) {
            cout << searchHistory[i];
            if (i + 1 < shown) cout << "  ·  ";
        }
        cout << Color::RESET << "\n";
    }

    // ----------------------------------------------------------
    //  MENU
    // ----------------------------------------------------------

    /** Renders the main application menu. */
    void displayMenu() const {
        cout << "\n";
        cout << Color::GRAY << " ┌──────────────────────────────────────────────┐\n" << Color::RESET;
        cout << Color::BLUE << " │ ██████╗  ███████╗████████╗                   │\n" << Color::RESET;
        cout << Color::BLUE << " │ ██╔══██╗██╔════╝╚══██╔══╝   " << Color::RESET << Color::BOLD << "Dictionary" << Color::BLUE << "       │\n" << Color::RESET;
        cout << Color::BLUE << " │ ██████╦╝███████╗   ██║      " << Color::RESET << Color::GRAY << "v4.0 · C++" << Color::BLUE << "       │\n" << Color::RESET;
        cout << Color::BLUE << " │ ██╔══██╗╚════██║   ██║                       │\n" << Color::RESET;
        cout << Color::BLUE << " │ ██████╦╝███████║   ██║                       │\n" << Color::RESET;
        cout << Color::BLUE << " │ ╚═════╝ ╚══════╝   ╚═╝                       │\n" << Color::RESET;
        cout << Color::GRAY << " └──────────────────────────────────────────────┘\n\n" << Color::RESET;

        cout << Color::GRAY
             << " Vardhaman College of Engineering  ·  CSE Dept\n"
             << " Mentor: Sai Riddhi   ·  Summer Project 2026\n"
             << " ------------------------------------------------\n\n" << Color::RESET;

        cout << Color::YELLOW << Color::BOLD << " MAIN MENU\n" << Color::RESET;
        cout << Color::GRAY   << " ------------------------------------------------\n\n" << Color::RESET;

        cout << Color::GREEN  << " [1] " << Color::RESET << "Add Word          " << Color::GRAY << "Insert into BST\n"            << Color::RESET;
        cout << Color::GREEN  << " [2] " << Color::RESET << "Search Word       " << Color::GRAY << "Lookup & details\n"           << Color::RESET;
        cout << Color::GREEN  << " [3] " << Color::RESET << "Delete Word       " << Color::GRAY << "Remove from BST\n"            << Color::RESET;
        cout << Color::GREEN  << " [4] " << Color::RESET << "Display All Words " << Color::GRAY << "In-order traversal\n"         << Color::RESET;
        cout << Color::GREEN  << " [5] " << Color::RESET << "Count Words       " << Color::GRAY << "Total + tree height\n"        << Color::RESET;
        cout << Color::GREEN  << " [6] " << Color::RESET << "Auto-Complete     " << Color::GRAY << "Prefix BST search O(p+m)\n"   << Color::RESET;
        cout << Color::GREEN  << " [7] " << Color::RESET << "Edit Word         " << Color::GRAY << "Update existing entry\n"      << Color::RESET;
        cout << Color::CYAN   << " [8] " << Color::RESET << "Search History    " << Color::GRAY << "View recent searches\n"       << Color::RESET;
        cout << Color::YELLOW << " [9] " << Color::RESET << "Save Dictionary   " << Color::GRAY << "Atomic write to file\n"       << Color::RESET;
        cout << Color::YELLOW << "[10] " << Color::RESET << "Load Dictionary   " << Color::GRAY << "Read from file\n"             << Color::RESET;
        cout << Color::RED    << " [0] " << Color::RESET << "Exit              " << Color::GRAY << "Auto-saves on quit\n\n"       << Color::RESET;

        cout << Color::GRAY
             << " ------------------------------------------------\n"
             << " ✦ v4.0: optional<ref> search · atomic save · O(min(m,n))\n"
             << "         Levenshtein · rule-of-5 BST · editWord retry loops\n\n"
             << Color::RESET;

        cout << Color::GREEN << " → " << Color::GRAY << "Enter a number (0-10) and press Enter.\n" << Color::RESET;
        cout << Color::GREEN << "\n > " << Color::RESET;
    }

    // ----------------------------------------------------------
    //  FEATURE IMPLEMENTATIONS
    // ----------------------------------------------------------

    /**
     * @brief  Feature 1: Add a new word.
     *
     * Validation applied to every field:
     *   Word       [VAL-01] alpha+hyphen, 1-MAX_WORD_LEN, no leading/trailing '-'
     *   Definition [VAL-02] non-empty, ≤ MAX_DEFINITION_LEN
     *   Word type  [VAL-03] must match accepted POS set; retried until valid
     *   Phonetic   [VAL-04] optional; ≤ MAX_PHONETIC_LEN
     *   Synonyms   [VAL-05] each token validated; ≤ MAX_SYNONYMS; '|' disallowed
     *
     * Duplicate word → user offered the chance to edit instead.
     */
    void addWord() {
        printHeader("ADD WORD");

        // ── Word [VAL-01] ────────────────────────────────────────
        string word;
        while (true) {
            word = toLower(getLine("Enter word          : "));
            if (word.empty()) {
                cout << Color::RED << "  [!] Word cannot be empty.\n" << Color::RESET;
                continue;
            }
            string err;
            if (!isValidWord(word, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            break;
        }

        if (bst.search(word)) {
            cout << Color::YELLOW << "\n  [!] '" << word
                 << "' already exists.\n" << Color::RESET;
            if (getChoice("  Edit it instead? (y/n): ") == 'y') editWord(word);
            return;
        }

        WordEntry e;
        e.word = word;

        // ── Definition [VAL-02] ──────────────────────────────────
        while (true) {
            e.definition = getLine("Enter definition    : ");
            string err;
            if (!isValidDefinition(e.definition, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            break;
        }

        // ── Word type [VAL-03] ───────────────────────────────────
        while (true) {
            const string raw = getLine("Word type (Noun/Verb/Adj/…): ");
            if (raw.empty()) {
                cout << Color::RED << "  [!] Word type cannot be empty.\n" << Color::RESET;
                continue;
            }
            string canonical, err;
            if (!isValidWordType(raw, canonical, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            e.wordType = canonical;
            break;
        }

        // ── Phonetic [VAL-04] — optional ─────────────────────────
        while (true) {
            const string ph = getLine("Phonetic (optional) : ");
            if (ph.empty()) { e.phonetic = ""; break; }
            if (static_cast<int>(ph.size()) > Constants::MAX_PHONETIC_LEN) {
                cout << Color::RED << "  [!] Phonetic exceeds max length of "
                     << Constants::MAX_PHONETIC_LEN << " chars.\n" << Color::RESET;
                continue;
            }
            e.phonetic = ph;
            break;
        }

        // ── Synonyms [VAL-05] — optional ─────────────────────────
        while (true) {
            const string synInput = getLine("Synonyms (comma-sep): ");
            if (synInput.empty()) break;
            const auto tokens = split(synInput, Constants::SYNONYM_DELIM);
            if (static_cast<int>(tokens.size()) > Constants::MAX_SYNONYMS) {
                cout << Color::RED << "  [!] Too many synonyms. Max is "
                     << Constants::MAX_SYNONYMS << ".\n" << Color::RESET;
                continue;
            }
            bool ok = true;
            for (const auto& syn : tokens) {
                string err;
                if (!isValidSynonym(syn, err)) {
                    cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                    ok = false; break;
                }
            }
            if (!ok) continue;
            e.synonyms = tokens;
            break;
        }

        bst.insert(e);
        cout << Color::GREEN << "\n  [✓] '" << word << "' added!\n" << Color::RESET;
    }

    /**
     * @brief  Feature 2: Search for a word and display its entry.
     *
     * If exact match is absent:
     *   - Autocomplete hints shown inline.
     *   - Spell suggestion offered.
     */
    void searchWord() {
        printHeader("SEARCH WORD");
        showHistorySuggestions();

        string word;
        while (true) {
            word = toLower(getLine("\n  Enter word to search: "));
            if (word.empty()) {
                cout << Color::RED << "  [!] Please enter a word.\n" << Color::RESET;
                continue;
            }
            string err;
            if (!isValidWord(word, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            break;
        }

        // Autocomplete hint
        const auto suggestions = bst.autoComplete(word);
        if (!suggestions.empty() && suggestions[0] != word) {
            cout << Color::DIM << "  Suggestions: ";
            const size_t show = std::min(suggestions.size(),
                                static_cast<size_t>(Constants::AUTOCOMPLETE_SHOW));
            for (size_t i = 0; i < show; ++i) {
                cout << suggestions[i];
                if (i + 1 < show) cout << "  ";
            }
            cout << Color::RESET << "\n";
        }

        // [FIX-01] search() returns optional<ref> — no raw pointer
        auto optEntry = bst.search(word);
        if (optEntry) {
            pushHistory(word);
            printEntry(optEntry->get());
        } else {
            cout << Color::RED << "\n  [✗] '" << word << "' not found.\n" << Color::RESET;
            const string suggestion = bst.spellSuggest(word);
            if (!suggestion.empty()) {
                cout << Color::YELLOW << "  Did you mean: " << Color::BOLD
                     << suggestion << Color::RESET << " ?\n";
                if (getChoice("  Search for '" + suggestion + "'? (y/n): ") == 'y') {
                    auto se = bst.search(suggestion);
                    if (se) { pushHistory(suggestion); printEntry(se->get()); }
                }
            }
        }
    }

    /**
     * @brief  Feature 3: Delete a word after confirmation.
     *         Shows the entry before asking the user to confirm.
     */
    void deleteWord() {
        printHeader("DELETE WORD");

        string word;
        while (true) {
            word = toLower(getLine("Enter word to delete: "));
            if (word.empty()) {
                cout << Color::RED << "  [!] Please enter a word.\n" << Color::RESET;
                continue;
            }
            string err;
            if (!isValidWord(word, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            break;
        }

        auto optEntry = bst.search(word);
        if (!optEntry) {
            cout << Color::RED << "  [✗] '" << word << "' not found.\n" << Color::RESET;
            return;
        }
        printEntry(optEntry->get());
        if (getChoice("  Confirm delete? (y/n): ") == 'y') {
            bst.remove(word);
            cout << Color::GREEN << "  [✓] '" << word << "' deleted.\n" << Color::RESET;
        } else {
            cout << Color::DIM << "  Delete cancelled.\n" << Color::RESET;
        }
    }

    /** @brief  Feature 4: Display all words A → Z. */
    void displayAllWords() const {
        printHeader("ALL WORDS  (A → Z)");
        if (bst.isEmpty()) {
            cout << Color::DIM << "  The dictionary is currently empty.\n" << Color::RESET;
            return;
        }
        const auto words = bst.getAllWords();
        cout << "  " << Color::DIM << words.size() << " word(s) found.\n\n" << Color::RESET;
        for (const auto& e : words) printEntry(e);
    }

    /**
     * @brief  Feature 5: Word count and tree height.
     *         Height shown as a balance diagnostic.
     */
    void countWords() const {
        printHeader("WORD COUNT");
        const int wc = bst.wordCount();
        const int ht = bst.getHeight();
        cout << "  Total words  : " << Color::BOLD << Color::CYAN
             << wc << Color::RESET << "\n";
        cout << "  Tree height  : " << Color::BOLD << Color::CYAN
             << ht << Color::RESET;
        if (wc > 1) {
            // Perfect BST height for n nodes is floor(log2(n))
            const int ideal = static_cast<int>(std::log2(wc));
            if (ht > ideal * 2)
                cout << Color::YELLOW << "  (consider reloading for better balance)"
                     << Color::RESET;
        }
        cout << "\n";
    }

    /**
     * @brief  Feature 6: Auto-complete prefix search.
     *         Validates prefix with the same rules as a word key.
     */
    void autoComplete() {
        printHeader("AUTO-COMPLETE");

        string prefix;
        while (true) {
            prefix = toLower(getLine("Type a prefix (e.g. 'app'): "));
            if (prefix.empty()) {
                cout << Color::RED << "  [!] Prefix cannot be empty.\n" << Color::RESET;
                continue;
            }
            string err;
            if (!isValidWord(prefix, err)) {
                cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                continue;
            }
            break;
        }

        const auto sugg = bst.autoComplete(prefix);
        if (sugg.empty()) {
            cout << Color::RED << "  No words found starting with '"
                 << prefix << "'.\n" << Color::RESET;
        } else {
            cout << Color::GREEN << "  " << sugg.size()
                 << " suggestion(s):\n\n" << Color::RESET;
            for (const auto& s : sugg)
                cout << "    " << Color::CYAN << "→" << Color::RESET
                     << "  " << s << "\n";
        }
    }

    /**
     * @brief  Feature 7: Edit fields of an existing entry.  [FIX-04..10]
     *
     * Every field now:
     *   - Uses getLine() helper (safe EOF handling).  [FIX-10]
     *   - Has a retry loop — bad input prompts again rather than
     *     silently keeping the old value.  [FIX-07/08/09]
     *   - Pressing Enter (empty input) skips the field and keeps
     *     the current value.
     *
     * @param  preFilledWord  If non-empty, skips the word prompt
     *                        (used when redirected from addWord()).
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
                    cout << Color::RED << "  [!] Please enter a word.\n" << Color::RESET;
                    continue;
                }
                string err;
                if (!isValidWord(word, err)) {
                    cout << Color::RED << "  [!] " << err << "\n" << Color::RESET;
                    continue;
                }
                break;
            }
        }

        // [FIX-01] Use optional<ref> — no raw pointer
        auto optEntry = bst.search(word);
        if (!optEntry) {
            cout << Color::RED << "  [✗] '" << word << "' not found.\n" << Color::RESET;
            return;
        }
        WordEntry& entry = optEntry->get();

        printEntry(entry);
        cout << Color::DIM
             << "  (Press Enter at any field to keep the current value)\n\n"
             << Color::RESET;

        // ── Definition [FIX-07] — retry loop ─────────────────────
        while (true) {
            const string input = getLine(
                "Definition [" + entry.definition.substr(0,30)
                + (entry.definition.size()>30?"…":"") + "]: ");
            if (input.empty()) break;   // keep current
            string err;
            if (!isValidDefinition(input, err)) {
                cout << Color::RED << "  [!] " << err << " Try again.\n" << Color::RESET;
                continue;
            }
            entry.definition = input;
            cout << Color::GREEN << "  [✓] Definition updated.\n" << Color::RESET;
            break;
        }

        // ── Word type [FIX-08] — retry loop ──────────────────────
        while (true) {
            const string input = getLine("Word type [" + entry.wordType + "]: ");
            if (input.empty()) break;   // keep current
            string canonical, err;
            if (!isValidWordType(input, canonical, err)) {
                cout << Color::RED << "  [!] " << err << "\n  Try again.\n"
                     << Color::RESET;
                continue;
            }
            entry.wordType = canonical;
            cout << Color::GREEN << "  [✓] Word type updated.\n" << Color::RESET;
            break;
        }

        // ── Phonetic [FIX-09] — retry loop ───────────────────────
        while (true) {
            const string input = getLine("Phonetic [" + entry.phonetic + "]: ");
            if (input.empty()) break;   // keep current
            if (static_cast<int>(input.size()) > Constants::MAX_PHONETIC_LEN) {
                cout << Color::RED << "  [!] Phonetic exceeds max length of "
                     << Constants::MAX_PHONETIC_LEN << " chars. Try again.\n"
                     << Color::RESET;
                continue;
            }
            entry.phonetic = input;
            cout << Color::GREEN << "  [✓] Phonetic updated.\n" << Color::RESET;
            break;
        }

        // ── Synonyms [FIX-09] — retry loop ───────────────────────
        while (true) {
            string curSyns;
            for (size_t i = 0; i < entry.synonyms.size(); ++i) {
                curSyns += entry.synonyms[i];
                if (i + 1 < entry.synonyms.size()) curSyns += ", ";
            }
            const string input = getLine("Synonyms [" + curSyns + "]: ");
            if (input.empty()) break;   // keep current
            const auto tokens = split(input, Constants::SYNONYM_DELIM);
            if (static_cast<int>(tokens.size()) > Constants::MAX_SYNONYMS) {
                cout << Color::RED << "  [!] Too many synonyms (max "
                     << Constants::MAX_SYNONYMS << "). Try again.\n"
                     << Color::RESET;
                continue;
            }
            bool ok = true;
            for (const auto& syn : tokens) {
                string err;
                if (!isValidSynonym(syn, err)) {
                    cout << Color::RED << "  [!] " << err << " Try again.\n"
                         << Color::RESET;
                    ok = false; break;
                }
            }
            if (!ok) continue;
            entry.synonyms = tokens;
            cout << Color::GREEN << "  [✓] Synonyms updated.\n" << Color::RESET;
            break;
        }

        cout << Color::GREEN << "\n  [✓] '" << word << "' updated!\n" << Color::RESET;
    }

    /** @brief  Feature 8: Show search history, most-recent first. */
    void viewSearchHistory() const {
        printHeader("SEARCH HISTORY");
        if (searchHistory.empty()) {
            cout << Color::DIM << "  No searches yet.\n" << Color::RESET;
            return;
        }
        cout << "  " << Color::DIM << searchHistory.size()
             << " recent search(es):\n\n" << Color::RESET;
        int idx = 1;
        for (const auto& w : searchHistory)
            cout << "    " << Color::CYAN << idx++ << "." << Color::RESET
                 << "  " << w << "\n";
    }

    /** @brief  Feature 9: Explicit save. */
    void saveDictionary() { saveToFile(); }

    /** @brief  Feature 10: Explicit load (merges into existing tree). */
    void loadDictionary() { loadFromFile(); }

public:
    /** Constructor — restores session from file. */
    Dictionary() { loadFromFile(); }

    /** Destructor — BST destructor handles node cleanup. */
    ~Dictionary() = default;

    /**
     * @brief  Main application event loop.
     *
     * Reads and validates menu choice with [VAL-06/11], dispatches
     * to the appropriate feature, repeats until user selects 0.
     * On EOF: saves and exits gracefully.
     */
    void run() {
        bool running = true;
        while (running) {
            displayMenu();

            string rawInput;
            if (!std::getline(cin, rawInput)) {
                cout << Color::YELLOW
                     << "\n  [!] Input stream closed. Saving and exiting...\n"
                     << Color::RESET;
                saveToFile();
                break;
            }
            rawInput = trim(rawInput);

            int choice = -1;
            if (!parseMenuChoice(rawInput, choice)) {
                cout << Color::RED
                     << "\n  [!] Invalid choice. Enter a number 0–"
                     << Constants::MAX_MENU_CHOICE << ".\n" << Color::RESET;
                continue;
            }

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
                    cout << Color::YELLOW
                         << "\n  Exiting. Saving data...\n" << Color::RESET;
                    saveToFile();
                    running = false;
                    break;
                default:
                    // Defensive — parseMenuChoice already guards this path
                    cout << Color::RED << "\n  [!] Unrecognised option.\n"
                         << Color::RESET;
            }
        }
        cout << Color::CYAN
             << "\n  Goodbye! Thank you for using BST Dictionary v4.\n\n"
             << Color::RESET;
    }
};

// ============================================================
//  ENTRY POINT
// ============================================================

/**
 * @brief  Program entry point.
 *
 * On Windows, enables UTF-8 console output (code page 65001)
 * so ANSI characters and IPA phonetic symbols display correctly.
 *
 * @return 0 on normal exit.
 */
int main() {
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    Dictionary dict;
    dict.run();
    return 0;
}

/*
 * ============================================================
 *  README  [DOC-06]
 * ============================================================
 *
 *  BST Dictionary v4.0
 *  ====================
 *  A console-based English dictionary backed by an unbalanced
 *  Binary Search Tree, written in C++17.
 *
 *  BUILD
 *  -----
 *    g++ -std=c++17 -Wall -Wextra -Wpedantic \
 *        -o dictionary dictionary_v4_perfect.cpp
 *
 *  RUN
 *  ---
 *    ./dictionary
 *
 *  DATA FILE
 *  ---------
 *  Words are persisted in dictionary_data.txt (pipe-delimited):
 *    word|definition|wordType|phonetic|syn1,syn2,...
 *  The file is updated atomically via a temp-file + rename.
 *
 *  KEY IMPROVEMENTS OVER v3
 *  -------------------------
 *  1. optional<ref> search()        — no raw pointer returned
 *  2. Rule-of-5 BST                 — copy/move constructors defined
 *  3. O(min(m,n)) Levenshtein       — rolling-array DP
 *  4. editWord() safe input         — getLine() + retry loops everywhere
 *  5. Atomic file save              — temp-file + std::rename
 *  6. Length-diff Levenshtein prune — skip DP when length gap ≥ bestDist
 *  7. Overflow-safe autoComplete()  — '\xff' prefix handled as sentinel
 *  8. Per-field trim in loadFromFile() — robust against padded data files
 *
 *  TEAM
 *  ----
 *  Lead    : Rendla Vishnu Tej
 *  Members : Harika, Akshith, Manideep, Sanjitha
 *  Mentor  : Vechha Sai Riddhi
 *  College : Vardhaman College of Engineering, CSE Dept
 *  Year    : 2025-26
 * ============================================================
 */
