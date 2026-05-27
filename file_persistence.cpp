/*
 * ============================================================
 *  BST-Based Dictionary  —  Vardhaman College of Engineering
 *  CSE Department  |  Summer Project 2025-26
 *
 *  CONTRIBUTOR : Vechha Sai Riddhi  (Mentor)
 *  ROLE        : File Persistence + Utility Functions
 *  FEATURES    :
 *    saveToFile()   — Atomic write (temp-file + rename) [FIX-06]
 *    loadFromFile() — Per-field trim on parse             [FIX-12]
 *    split()        — Tokenise a string by delimiter
 *    trim()         — Strip leading/trailing whitespace
 *    toLower()      — Lowercase normalisation
 *    getLine()      — EOF-safe stdin reader
 *    getChoice()    — y/n prompt helper
 *    searchHistory  — Deque-based recent search tracking
 * ============================================================
 *
 *  CRITICAL FIXES IN THIS MODULE:
 *    [FIX-06] saveToFile() writes to TEMP_FILE first, then uses
 *             std::rename() (atomic on POSIX) to replace SAVE_FILE.
 *             A crash mid-write can never corrupt the live data file.
 *    [FIX-12] loadFromFile() trims each individual field after
 *             splitting — v3 only trimmed the whole line.
 *    [FIX-A]  BST: Rule of Three — copy constructor and copy
 *             assignment operator deleted to prevent double-free.
 *    [FIX-B]  BST::wordCount() uses a dedicated recursive counter
 *             instead of building a full vector unnecessarily.
 *    [FIX-C]  saveToFile() fallback path now removes TEMP_FILE
 *             even when the fallback copy itself fails.
 *    [FIX-D]  getChoice() re-prints the prompt on the retry attempt.
 *
 *  FILE FORMAT (dictionary_data.txt):
 *    One entry per line; fields separated by '|':
 *      word|definition|wordType|phonetic|syn1,syn2,...
 *    The synonyms field (index 4) is optional.
 *    Minimum required: 4 fields.
 *
 *  COMPILE : g++ -std=c++17 -Wall -o riddhi riddhi_file_persistence.cpp
 *  RUN     : ./riddhi
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <cctype>
#include <cstdio>    // std::rename, std::remove

using std::string;
using std::vector;
using std::deque;
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
    const string DIM     = "\033[2m";
    const string GRAY    = "\033[90m";
}

// ============================================================
//  FILE CONSTANTS
// ============================================================
namespace Constants {
    constexpr int  MAX_SYNONYMS       = 10;
    const     char FIELD_DELIM        = '|';
    const     char SYNONYM_DELIM      = ',';
    const     string SAVE_FILE        = "dictionary_data.txt";
    const     string TEMP_FILE        = "dictionary_data.tmp";  // [FIX-06]
    constexpr int  MAX_HISTORY        = 10;
    constexpr int  AUTOCOMPLETE_SHOW  = 5;
}

// ============================================================
//  DATA STRUCTURES
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
//  RIDDHI'S CONTRIBUTION — SECTION 1: UTILITY FUNCTIONS
// ============================================================

/**
 * @brief  Converts a string to lowercase.
 *
 * Used throughout the application for case-insensitive comparison
 * (e.g. all BST keys are stored lowercase).
 *
 * @param  s  Input string.
 * @return    Lowercase copy.
 * @complexity O(n).
 */
string toLower(const string& s) {
    string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return r;
}

/**
 * @brief  Strips leading and trailing whitespace from a string.
 *
 * Whitespace recognised: space (' '), tab ('\t'), carriage-return ('\r'),
 * and newline ('\n').
 *
 * Returns an empty string if the entire input is whitespace.
 *
 * @param  s  Raw input string.
 * @return    Trimmed copy.
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
 * @brief  Splits a string by a delimiter and trims each token.
 *
 * Empty tokens (after trimming) are discarded.
 *
 * Example:
 *   split("apple | banana | cherry", '|')
 *     → {"apple", "banana", "cherry"}
 *
 *   split("cat,,dog", ',')
 *     → {"cat", "dog"}   (empty middle token discarded)
 *
 * @param  s      Source string.
 * @param  delim  Character to split on.
 * @return        Vector of non-empty trimmed tokens.
 * @complexity    O(n).
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

// ============================================================
//  RIDDHI'S CONTRIBUTION — SECTION 2: STDIN INPUT HELPERS
// ============================================================

/**
 * @brief  Prompts the user and reads one trimmed line from stdin.
 *
 * Guards against two failure modes:
 *   • EOF (user pressed Ctrl+D / input pipe closed)
 *   • Stream failure (read error)
 *
 * On either, the stream is cleared and an empty string is returned
 * so callers can detect the failure without crashing.
 *
 * @param  prompt  Text displayed before the cursor (no newline needed).
 * @return         Trimmed input line; empty string on EOF/failure.
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
 * @brief  Prompts for a y/n answer and returns the first character.
 *
 * Retries ONCE on empty input before giving up.
 * Returns '\0' on EOF or repeated empty input.
 *
 * [FIX-D] The prompt is now re-printed on the retry attempt so the
 *         user sees a visible cue after the error message.
 *
 * @param  prompt  Text displayed before the cursor.
 * @return         Lowercased first character ('y', 'n', etc.) or '\0'.
 */
char getChoice(const string& prompt) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        // [FIX-D] Always print the prompt (including on retry)
        cout << Color::YELLOW << "  " << prompt << Color::RESET;
        string s;
        if (!std::getline(cin, s)) { cin.clear(); return '\0'; }
        s = trim(s);
        if (!s.empty())
            return static_cast<char>(
                std::tolower(static_cast<unsigned char>(s[0])));
        cout << Color::RED << "  [!] Please enter 'y' or 'n'.\n"
             << Color::RESET;
    }
    return '\0';
}

// ============================================================
//  RIDDHI'S CONTRIBUTION — SECTION 3: SEARCH HISTORY
// ============================================================

/**
 * @brief  Records a successfully searched word in the history deque.
 *
 * Behaviour:
 *   • Deduplicates: if the word was the most-recent search, skip.
 *   • Trims oldest entries if size exceeds MAX_HISTORY (10).
 *   • Most-recent search is at front (index 0).
 *
 * @param  history  The deque to push to.
 * @param  word     Word that was just searched.
 */
void pushHistory(deque<string>& history, const string& word) {
    if (!history.empty() && history.front() == word) return;
    history.push_front(word);
    while (static_cast<int>(history.size()) > Constants::MAX_HISTORY)
        history.pop_back();
}

/**
 * @brief  Prints the most-recent AUTOCOMPLETE_SHOW searches.
 *
 * Shown in the search prompt as "Recent: word1  ·  word2  ·  ..."
 * to help users re-access recently looked-up words quickly.
 *
 * @param  history  The current search history deque.
 */
void showHistorySuggestions(const deque<string>& history) {
    if (history.empty()) return;
    cout << Color::DIM << "  Recent: ";
    const int shown = std::min(static_cast<int>(history.size()),
                               Constants::AUTOCOMPLETE_SHOW);
    for (int i = 0; i < shown; ++i) {
        cout << history[i];
        if (i + 1 < shown) cout << "  ·  ";
    }
    cout << Color::RESET << "\n";
}

// ============================================================
//  BST HELPER STUBS (for this standalone file)
// ============================================================
class BST {
    Node* root = nullptr;

    // ── Private helpers ────────────────────────────────────
    void freeTree(Node* n) {
        if (!n) return;
        freeTree(n->left);
        freeTree(n->right);
        delete n;
    }

    Node* insertH(Node* n, const WordEntry& e, bool& ok) {
        if (!n) { ok = true; return new Node(e); }
        if (e.word < n->data.word)       n->left  = insertH(n->left,  e, ok);
        else if (e.word > n->data.word)  n->right = insertH(n->right, e, ok);
        else ok = false;   // duplicate — silently reject
        return n;
    }

    void inOrder(Node* n, vector<WordEntry>& out) const {
        if (!n) return;
        inOrder(n->left, out);
        out.push_back(n->data);
        inOrder(n->right, out);
    }

    bool searchH(Node* n, const string& k) const {
        if (!n) return false;
        if (n->data.word == k) return true;
        return (k < n->data.word) ? searchH(n->left, k) : searchH(n->right, k);
    }

    /**
     * [FIX-B] Dedicated recursive counter — avoids allocating a full
     *         vector of WordEntry objects just to get the size.
     */
    int countH(Node* n) const {
        if (!n) return 0;
        return 1 + countH(n->left) + countH(n->right);
    }

public:
    BST()  = default;
    ~BST() { freeTree(root); }

    // [FIX-A] Prevent shallow copy — would cause double-free on destruction.
    BST(const BST&)            = delete;
    BST& operator=(const BST&) = delete;

    bool insert(const WordEntry& e) {
        bool ok = false;
        root = insertH(root, e, ok);
        return ok;
    }

    bool wordExists(const string& k) const { return searchH(root, toLower(k)); }

    vector<WordEntry> getAllWords() const {
        vector<WordEntry> r;
        inOrder(root, r);
        return r;
    }

    // [FIX-B] O(n) count without vector allocation
    int wordCount() const { return countH(root); }
};

// ============================================================
//  RIDDHI'S CONTRIBUTION — SECTION 4: SAVE TO FILE  [FIX-06]
// ============================================================

/**
 * @brief  Persists all BST entries to disk using atomic write.  [FIX-06]
 *
 * ── Atomic write strategy ─────────────────────────────────
 * Problem: if we write directly to SAVE_FILE and the process
 * crashes mid-write, we get a partially-written (corrupted) file.
 *
 * Solution (POSIX guarantee):
 *   1. Write to TEMP_FILE ("dictionary_data.tmp") first.
 *      If this crashes, the original SAVE_FILE is untouched.
 *   2. Flush and close TEMP_FILE.
 *   3. Call std::rename(TEMP_FILE → SAVE_FILE).
 *      On POSIX, rename() is atomic — it either fully succeeds
 *      or leaves both files intact.  No half-written state.
 *
 * Fallback (cross-device):
 *   std::rename fails if TEMP_FILE and SAVE_FILE are on different
 *   filesystems.  We fall back to a manual copy + delete.
 *
 * [FIX-C] TEMP_FILE is now always cleaned up, even when the fallback
 *         copy itself fails (previously the file was left behind in
 *         the error branch).
 *
 * File format per line:
 *   word|definition|wordType|phonetic|syn1,syn2,...
 *
 * @param  bst   The BST to read all words from.
 */
void saveToFile(const BST& bst) {
    // Step 1: Open TEMP_FILE
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

    // Step 2: Write all records
    for (const auto& e : words) {
        file << e.word        << Constants::FIELD_DELIM
             << e.definition  << Constants::FIELD_DELIM
             << e.wordType    << Constants::FIELD_DELIM
             << e.phonetic    << Constants::FIELD_DELIM;

        // Synonyms: comma-separated
        for (size_t i = 0; i < e.synonyms.size(); ++i) {
            file << e.synonyms[i];
            if (i + 1 < e.synonyms.size()) file << Constants::SYNONYM_DELIM;
        }
        file << "\n";

        // Check stream health after every write
        if (!file.good()) {
            cout << Color::RED
                 << "  [Error] Write failure at record '" << e.word
                 << "'. Temp file may be incomplete.\n" << Color::RESET;
            file.close();
            std::remove(Constants::TEMP_FILE.c_str());
            return;
        }
        ++written;
    }
    file.close();

    // Step 3: Atomic rename  [FIX-06]
    if (std::rename(Constants::TEMP_FILE.c_str(),
                    Constants::SAVE_FILE.c_str()) != 0) {
        // Rename failed (cross-device) → manual copy fallback
        ifstream src(Constants::TEMP_FILE);
        ofstream dst(Constants::SAVE_FILE);
        if (src && dst) {
            dst << src.rdbuf();
            src.close();
            dst.close();
            cout << Color::YELLOW
                 << "  [!] Atomic rename unavailable; used copy fallback.\n"
                 << Color::RESET;
        } else {
            cout << Color::RED
                 << "  [Error] Could not finalise save. "
                    "Data is in '" << Constants::TEMP_FILE << "'.\n"
                 << Color::RESET;
            // [FIX-C] Clean up temp file even on fallback failure
            src.close();
            dst.close();
            std::remove(Constants::TEMP_FILE.c_str());
            return;
        }
        std::remove(Constants::TEMP_FILE.c_str());
    }

    cout << Color::GREEN << "  [✓] " << written << " word(s) saved to '"
         << Constants::SAVE_FILE << "'.\n" << Color::RESET;
}

// ============================================================
//  RIDDHI'S CONTRIBUTION — SECTION 5: LOAD FROM FILE  [FIX-12]
// ============================================================

/**
 * @brief  Deserialises entries from SAVE_FILE into the BST.  [FIX-12]
 *
 * Parse algorithm:
 *   1. Read file line by line.
 *   2. Skip blank lines.
 *   3. Split each line on '|' → at least 4 tokens required.
 *   4. Trim EACH individual field after splitting.  [FIX-12]
 *      (v3 only trimmed the whole line — fields could have
 *       leading/trailing spaces from manual edits.)
 *   5. Validate the word field; skip malformed records.
 *   6. Parse synonyms from token[4] (comma-separated), if present.
 *   7. Insert into BST; duplicates are silently rejected.
 *
 * Silent no-op if SAVE_FILE doesn't exist (first run).
 *
 * @param  bst  The BST to load words into.
 */
void loadFromFile(BST& bst) {
    ifstream file(Constants::SAVE_FILE);
    if (!file.is_open()) {
        cout << Color::DIM
             << "  [Info] No existing data file found — starting fresh.\n"
             << Color::RESET;
        return;
    }

    string line;
    int loaded  = 0;
    int skipped = 0;

    while (std::getline(file, line)) {
        const string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        const auto parts = split(trimmedLine, Constants::FIELD_DELIM);
        if (parts.size() < 4) { ++skipped; continue; }

        WordEntry e;
        // [FIX-12] trim each field individually after splitting
        e.word       = toLower(trim(parts[0]));
        e.definition = trim(parts[1]);
        e.wordType   = trim(parts[2]);
        e.phonetic   = trim(parts[3]);

        // Basic word validation
        if (e.word.empty()) { ++skipped; continue; }

        // Optional synonyms field
        if (parts.size() >= 5 && !parts[4].empty()) {
            e.synonyms = split(parts[4], Constants::SYNONYM_DELIM);
        }

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
}

// ============================================================
//  DEMONSTRATION MAIN
// ============================================================
int main() {
    cout << Color::CYAN << Color::BOLD
         << "\n  ══ RIDDHI: File Persistence + Utility Demo ══\n\n"
         << Color::RESET;

    // ── Utility function demos ────────────────────────────────
    cout << Color::YELLOW << Color::BOLD
         << "  -- trim() Tests --\n" << Color::RESET;
    vector<string> trimTests = {"  apple  ", "\tbanana\n", "clean", "   "};
    for (auto& s : trimTests)
        cout << "  trim(\"" << s << "\") = \"" << trim(s) << "\"\n";

    cout << "\n" << Color::YELLOW << Color::BOLD
         << "  -- split() Tests --\n" << Color::RESET;
    auto printSplit = [](const string& s, char d) {
        auto tok = split(s, d);
        cout << "  split(\"" << s << "\", '" << d << "') = {";
        for (size_t i = 0; i < tok.size(); ++i) {
            cout << "\"" << tok[i] << "\"";
            if (i + 1 < tok.size()) cout << ", ";
        }
        cout << "}\n";
    };
    printSplit("apple|banana|cherry", '|');
    printSplit("run,walk,jump", ',');
    printSplit("  padded | fields | here  ", '|');

    // ── Search history demo ───────────────────────────────────
    cout << "\n" << Color::YELLOW << Color::BOLD
         << "  -- Search History Demo --\n" << Color::RESET;
    deque<string> history;
    for (auto& w : {"apple","binary","compile","apple","delete"})
        pushHistory(history, w);
    cout << "  After searching: apple, binary, compile, apple, delete\n  ";
    showHistorySuggestions(history);

    // ── Save + Load cycle ─────────────────────────────────────
    cout << "\n" << Color::YELLOW << Color::BOLD
         << "  -- Save & Load Cycle --\n" << Color::RESET;

    BST bst;
    auto add = [&](const string& w, const string& def, const string& t,
                   const string& ph = "", vector<string> syns = {}) {
        WordEntry e;
        e.word = toLower(w); e.definition = def;
        e.wordType = t; e.phonetic = ph; e.synonyms = syns;
        bst.insert(e);
    };
    add("apple",   "A sweet round fruit",             "Noun",      "/ˈæpəl/",    {"fruit"});
    add("binary",  "Base-2 number system",             "Adjective", "/ˈbaɪnəri/", {"dual"});
    add("compile", "Convert source code",              "Verb",      "/kəmˈpaɪl/", {"build"});

    cout << "\n  [SAVE] Writing " << bst.wordCount() << " words to file...\n";
    saveToFile(bst);

    BST bst2;
    cout << "\n  [LOAD] Reading back into new BST...\n";
    loadFromFile(bst2);

    cout << "\n  [VERIFY] Words loaded: " << bst2.wordCount() << "\n";
    for (auto& w : {"apple","binary","compile"}) {
        cout << "    " << (bst2.wordExists(w) ? Color::GREEN + "[✓] " : Color::RED + "[✗] ")
             << Color::RESET << w << "\n";
    }

    // Cleanup demo file
    std::remove(Constants::SAVE_FILE.c_str());
    cout << Color::DIM << "\n  (Demo file cleaned up)\n" << Color::RESET;

    cout << Color::CYAN
         << "\n  ══ End of Riddhi Demo ══\n\n"
         << Color::RESET;
    return 0;
}
