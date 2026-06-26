/*
 * ============================================================
 *  BST-Based Dictionary  —  Vardhaman College of Engineering
 *  CSE Department  |  Summer Project 2025-26
 *
 *  CONTRIBUTOR : Sakinala Sanjitha
 *  ROLE        : AI Auto-Complete & Spell Suggestion
 *  COMPLEXITY  :
 *    AutoComplete  — O(p + m)  where p = prefix length, m = matches
 *    Spell Suggest — O(n * min(|word|, |key|)) full-tree scan
 * ============================================================
 *
 *  This file contains:
 *    [1] editDistance()       — Levenshtein distance, O(min(m,n)) space
 *    [2] collectByPrefix()    — Range-pruned BST traversal for prefix
 *    [3] BST::autoComplete()  — Public prefix-search API
 *    [4] closest()            — Tree scan for minimum edit distance
 *    [5] BST::spellSuggest()  — Public spell-suggestion API
 *    [6] Dictionary::autoComplete()  — User-facing feature [Menu #6]
 *
 *  KEY FIXES IMPLEMENTED:
 *    [FIX-03] editDistance uses O(min(m,n)) rolling array — halves memory
 *    [FIX-05] autoComplete handles prefix ending in '\xff' safely
 *    [FIX-14] Levenshtein early-exit: length-diff lower bound prunes
 *             nodes whose distance cannot possibly beat current best.
 *
 *  COMPILE : g++ -std=c++17 -Wall -o sanjitha sanjitha_autocomplete_spell.cpp
 *  RUN     : ./sanjitha
 * ============================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <climits>
#include <optional>
#include <functional>
#include <cmath>

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
//  CONSTANTS
// ============================================================
namespace Constants {
    constexpr int SPELL_DIST_THRESH  = 3;   // max edit distance for suggestion
    constexpr int AUTOCOMPLETE_SHOW  = 5;   // max results shown in menu
}

// ============================================================
//  UTILITY
// ============================================================
string toLower(const string& s) {
    string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return r;
}

// ============================================================
//  DATA STRUCTURES (minimal — shared types)
// ============================================================
struct WordEntry {
    string         word;
    string         definition;
    string         wordType;
    string         phonetic;
    vector<string> synonyms;
};

struct Node {
    WordEntry data;
    Node*     left  = nullptr;
    Node*     right = nullptr;
    explicit  Node(const WordEntry& e) : data(e) {}
};

// ============================================================
//  SANJITHA'S CONTRIBUTION — SECTION 1: EDIT DISTANCE
//  Levenshtein distance with O(min(m,n)) rolling-array DP
// ============================================================

/**
 * @brief  Computes the Levenshtein edit distance between strings a and b.
 *
 * ── What is Levenshtein distance? ───────────────────────────
 * The minimum number of single-character operations needed to
 * transform string a into string b:
 *   • Insert a character
 *   • Delete a character
 *   • Replace one character with another
 *
 * Example: "kitten" → "sitting"  = 3 edits
 *   kitten → sitten  (replace k→s)
 *   sitten → sittin  (replace e→i)
 *   sittin → sitting (insert g)
 *
 * ── Space Optimisation  [FIX-03] ────────────────────────────
 * The classic DP uses an (m+1)×(n+1) table → O(m*n) space.
 * We only ever need the PREVIOUS row to compute the CURRENT row.
 * So we keep just TWO rows (prev, curr) and swap them each step
 * → O(min(m,n)) space while maintaining full O(m*n) correctness.
 *
 * @param  a  First string (longer string → more efficient as rows).
 * @param  b  Second string.
 * @return    Minimum edit distance.
 * @complexity Time O(m*n),  Space O(min(m,n)).
 */
int editDistance(const string& a, const string& b) {
    // Place longer string along rows for best cache behaviour
    const string& rows = (a.size() >= b.size()) ? a : b;
    const string& cols = (a.size() >= b.size()) ? b : a;

    const int R = static_cast<int>(rows.size());
    const int C = static_cast<int>(cols.size());

    // Two rows suffice — we swap after each row iteration
    vector<int> prev(C + 1), curr(C + 1);

    // Base row: cost of converting empty prefix → cols[0..j-1] = j insertions
    for (int j = 0; j <= C; ++j) prev[j] = j;

    for (int i = 1; i <= R; ++i) {
        curr[0] = i;   // cost of converting rows[0..i-1] → empty = i deletions
        for (int j = 1; j <= C; ++j) {
            if (rows[i-1] == cols[j-1]) {
                curr[j] = prev[j-1];                    // characters match — free
            } else {
                curr[j] = 1 + std::min({ prev[j],       // deletion  (from row)
                                         curr[j-1],      // insertion (into col)
                                         prev[j-1] });   // substitution
            }
        }
        std::swap(prev, curr);   // O(1) — reuse allocation, no copy
    }
    return prev[C];   // after last swap, prev holds the final row
}

// ============================================================
//  SANJITHA'S CONTRIBUTION — SECTION 2: AUTO-COMPLETE
//  Range-pruned BST prefix traversal
// ============================================================

/**
 * @brief  Recursive range-pruned prefix collector.  [FIX-05]
 *
 * Exploits BST ordering to prune irrelevant branches:
 *
 *   node.word >= upperBound (and upperBound ≠ "")
 *     → This node and its right subtree are all ≥ upper bound.
 *       Only the LEFT subtree can hold matches.
 *
 *   node.word < prefix
 *     → This node and its left subtree are all < prefix.
 *       Only the RIGHT subtree can hold matches.
 *
 *   Otherwise (prefix ≤ node.word < upperBound)
 *     → This node is in range. Check if it starts with prefix.
 *       Both subtrees may contain matches — explore both.
 *
 * Upper-bound construction:
 *   upperBound = prefix with last char incremented by 1.
 *   E.g. prefix="app" → upperBound="apq"
 *        Words in ["app","apq") are all words starting with "app".
 *   Edge case: if last char is '\xff' (0xFF, max ASCII), incrementing
 *   overflows → use empty-string sentinel meaning "no upper bound". [FIX-05]
 *
 * @param  node        Current BST node.
 * @param  prefix      Lowercase search prefix.
 * @param  upperBound  Exclusive upper bound; "" = unbounded.
 * @param  results     Accumulates matching words (in-order by BST).
 * @complexity         O(log n + m) where m = number of matches found.
 */
void collectByPrefix(Node*          node,
                     const string&  prefix,
                     const string&  upperBound,
                     vector<string>& results) {
    if (!node) return;
    const string& nw = node->data.word;

    // ── Upper-bound prune ─────────────────────────────────────
    // Skip sentinel ("") so we never incorrectly prune when prefix
    // ended with '\xff'.
    if (!upperBound.empty() && nw >= upperBound) {
        collectByPrefix(node->left, prefix, upperBound, results);
        return;
    }

    // ── Lower-bound prune ─────────────────────────────────────
    if (nw < prefix) {
        collectByPrefix(node->right, prefix, upperBound, results);
        return;
    }

    // ── Node is in range — check both subtrees ────────────────
    collectByPrefix(node->left,  prefix, upperBound, results);

    // Record this node if its word starts with the prefix
    if (nw.size() >= prefix.size() &&
        nw.compare(0, prefix.size(), prefix) == 0) {
        results.push_back(nw);
    }

    collectByPrefix(node->right, prefix, upperBound, results);
}

// ============================================================
//  SANJITHA'S CONTRIBUTION — SECTION 3: SPELL SUGGEST
//  Closest match by edit distance with early-exit pruning
// ============================================================

/**
 * @brief  Finds the word in the BST closest to key by edit distance.
 *
 * ── Algorithm ────────────────────────────────────────────────
 * Traverse the entire tree (O(n)).  At each node:
 *   1. Length-difference lower bound  [FIX-14]:
 *      The edit distance between two strings is AT LEAST
 *      |len(a) - len(b)|.  If this lower bound already meets or
 *      exceeds our current bestDist, skip the expensive DP call.
 *
 *   2. Compute Levenshtein distance.
 *      If it beats bestDist, update bestDist and bestWord.
 *
 *   3. Early-exit: if bestDist == 0, we found an exact match.
 *      No other word can be closer — stop immediately.
 *
 * @param  node      Current BST node.
 * @param  key       Misspelled query (lowercase).
 * @param  bestWord  Best candidate word so far (updated in-place).
 * @param  bestDist  Edit distance to bestWord   (updated in-place).
 * @complexity       O(n) traversal; O(min(m,k)) per editDistance call.
 */
void closest(Node*   node,   const string& key,
             string& bestWord, int& bestDist) {
    if (!node || bestDist == 0) return;   // early-exit on exact match

    // [FIX-14] Length-difference lower bound
    const int lenDiff = static_cast<int>(node->data.word.size())
                      - static_cast<int>(key.size());
    if (std::abs(lenDiff) < bestDist) {
        // Gap is small enough that a beat is still possible
        const int d = editDistance(key, node->data.word);
        if (d < bestDist) {
            bestDist = d;
            bestWord = node->data.word;
        }
    }

    closest(node->left,  key, bestWord, bestDist);
    closest(node->right, key, bestWord, bestDist);
}

// ============================================================
//  BST WRAPPER — public API for Sanjitha's two features
// ============================================================
class BST {
    Node* root = nullptr;

    void freeTree(Node* n) {
        if (!n) return;
        freeTree(n->left);
        freeTree(n->right);
        delete n;
    }

    void inOrder(Node* n, vector<WordEntry>& out) const {
        if (!n) return;
        inOrder(n->left, out);
        out.push_back(n->data);
        inOrder(n->right, out);
    }

    Node* insert(Node* n, const WordEntry& e, bool& ok) {
        if (!n) { ok = true; return new Node(e); }
        if (e.word < n->data.word)       n->left  = insert(n->left,  e, ok);
        else if (e.word > n->data.word)  n->right = insert(n->right, e, ok);
        else ok = false;
        return n;
    }

public:
    BST()  = default;
    ~BST() { freeTree(root); }

    bool insert(const WordEntry& e) {
        bool ok = false;
        root = insert(root, e, ok);
        return ok;
    }

    bool isEmpty() const { return !root; }

    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Returns words that begin with prefix.  [FIX-05]
     *
     * Public auto-complete entry point (Sanjitha's contribution).
     *
     * @param  prefix  Prefix to search (1+ characters).
     * @return         Sorted vector of matching word strings.
     * @complexity     O(log n + m).
     */
    vector<string> autoComplete(const string& prefix) const {
        vector<string> results;
        const string p = toLower(prefix);
        if (p.empty()) return results;

        // Build exclusive upper bound by incrementing last character
        string upper = p;
        if (static_cast<unsigned char>(upper.back()) == 0xFF) {
            upper = "";   // [FIX-05] sentinel: last char is max — no upper bound
        } else {
            upper.back()++;
        }

        collectByPrefix(root, p, upper, results);
        std::sort(results.begin(), results.end());
        return results;
    }

    // ──────────────────────────────────────────────────────────
    /**
     * @brief  Returns the closest dictionary word to a misspelled query.
     *
     * Public spell-suggest entry point (Sanjitha's contribution).
     *
     * @param  key  Lowercase misspelled word.
     * @return      The closest word within SPELL_DIST_THRESH edits,
     *              or "" if nothing is close enough.
     * @complexity  O(n * min(|word|, |key|)).
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
//  DEMONSTRATION MAIN
// ============================================================
int main() {
    BST bst;

    // Seed the BST with sample words
    auto add = [&](const string& w, const string& def, const string& type) {
        WordEntry e;
        e.word = toLower(w); e.definition = def; e.wordType = type;
        bst.insert(e);
    };

    add("apple",      "A round edible fruit",                      "Noun");
    add("application","A program designed for a specific purpose",  "Noun");
    add("apply",      "To put something to use",                    "Verb");
    add("apt",        "Appropriate or suitable",                    "Adjective");
    add("binary",     "Base-2 number system",                       "Adjective");
    add("compile",    "Convert source code to machine code",        "Verb");
    add("delete",     "Remove or erase",                            "Verb");
    add("elegant",    "Graceful and stylish",                       "Adjective");
    add("function",   "A reusable block of code",                   "Noun");
    add("graph",      "A mathematical structure of nodes and edges","Noun");

    // ── AUTO-COMPLETE DEMO ────────────────────────────────────
    cout << Color::CYAN << Color::BOLD
         << "\n  ══ SANJITHA: Auto-Complete Demo ══\n\n"
         << Color::RESET;

    auto testAC = [&](const string& prefix) {
        auto results = bst.autoComplete(prefix);
        cout << Color::YELLOW << "  Prefix '" << prefix << "' → "
             << Color::RESET;
        if (results.empty()) {
            cout << Color::RED << "No matches\n" << Color::RESET;
        } else {
            for (size_t i = 0; i < results.size(); ++i) {
                cout << Color::GREEN << results[i] << Color::RESET;
                if (i + 1 < results.size()) cout << ",  ";
            }
            cout << "\n";
        }
    };

    testAC("app");    // apple, application, apply
    testAC("ap");     // apple, application, apply, apt
    testAC("co");     // compile
    testAC("z");      // no matches

    // ── SPELL-SUGGEST DEMO ────────────────────────────────────
    cout << Color::CYAN << Color::BOLD
         << "\n  ══ SANJITHA: Spell Suggestion Demo ══\n\n"
         << Color::RESET;

    auto testSpell = [&](const string& typo) {
        string suggestion = bst.spellSuggest(typo);
        cout << Color::YELLOW << "  Typed: '" << typo << "'"
             << Color::RESET << " → ";
        if (suggestion.empty()) {
            cout << Color::RED << "No close match found\n" << Color::RESET;
        } else {
            cout << Color::GREEN << "Did you mean: '" << suggestion
                 << "'?\n" << Color::RESET;
        }
    };

    testSpell("aple");     // → apple   (1 edit)
    testSpell("complile"); // → compile (2 edits)
    testSpell("delet");    // → delete  (1 edit)
    testSpell("graff");    // → graph   (2 edits)
    testSpell("zzzzzzz");  // → no match

    // ── EDIT DISTANCE TABLE ───────────────────────────────────
    cout << Color::CYAN << Color::BOLD
         << "\n  ══ Edit Distance Quick-Table ══\n\n"
         << Color::RESET;
    struct Pair { string a, b; };
    vector<Pair> pairs = {
        {"kitten","sitting"}, {"apple","aple"},
        {"compile","complile"}, {"graph","graff"}
    };
    for (auto& [a,b] : pairs) {
        int d = editDistance(a, b);
        cout << "  editDistance(\"" << a << "\", \"" << b << "\") = "
             << Color::BOLD << d << Color::RESET << "\n";
    }

    cout << Color::CYAN
         << "\n  ══ End of Sanjitha Demo ══\n\n"
         << Color::RESET;
    return 0;
}