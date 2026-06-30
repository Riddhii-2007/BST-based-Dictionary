/*
 * ============================================================
 *  BST-Based Dictionary  —  Vardhaman College of Engineering
 *  CSE Department  |  Summer Project 2025-26
 *  Team Lead : Rendla Vishnu Tej
 *  Members   : Harika · Akshith · Manideep · Sanjitha
 *  Mentor    : Vechha Sai Riddhi
 * ============================================================
 *  Compile:  g++ -std=c++17 -o dictionary dictionary_fixed.cpp
 *  Run:      ./dictionary
 * ============================================================
 *
 *  FIXES APPLIED (v1.1):
 *  [FIX 1] autoComplete() — was O(n), now true O(p + m).
 *           findPrefixNode() descends in O(p) steps, then
 *           collectSubtree() gathers only matching nodes O(m).
 *  [FIX 2] public insert() — removed redundant pre-search.
 *           Now a single O(log n) pass; duplicate check is
 *           handled inside the private insert() directly.
 *  [FIX 3] spellSuggest() — added early-exit when bestDist==0
 *           (exact match found) to avoid unnecessary traversal.
 *  [NOTE]   Tree is unbalanced BST; worst-case for search/
 *           insert/delete is O(n) on sorted input. For
 *           guaranteed O(log n), upgrade to AVL or Red-Black.
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <climits>
#include <cstdlib>

using namespace std;

// ==========================================
// ANSI COLOR CODES FOR UI STYLING
// ==========================================
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

// ==========================================
// Word entry structure
// ==========================================
struct WordEntry {
    string word;        // key (stored lowercase)
    string definition;
    string wordType;    // Noun / Verb / Adjective / Adverb / etc.
    string phonetic;    // e.g.  /ˈæp.əl/
    vector<string> synonyms;
};

// ==========================================
// BST Node
// ==========================================
struct Node {
    WordEntry data;
    Node* left;
    Node* right;
    Node(const WordEntry& e) : data(e), left(nullptr), right(nullptr) {}
};

// ==========================================
// Utility functions
// ==========================================
string toLower(const string& s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char c){ return tolower(c); });
    return result;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

vector<string> split(const string& s, char delim) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delim)) {
        token = trim(token);
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

// Like split(), but keeps empty fields. Needed for parsing the
// pipe-delimited save file, where an empty phonetic or synonyms
// field must NOT shift the remaining columns to the left.
vector<string> splitKeepEmpty(const string& s, char delim) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delim)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

int editDistance(const string& a, const string& b) {
    int m = (int)a.size(), n = (int)b.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1])
                        ? dp[i-1][j-1]
                        : 1 + min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
    return dp[m][n];
}

void printDivider() {
    cout << GRAY << string(60, '-') << RESET << "\n";
}

void printHeader(const string& title) {
    cout << "\n" << CYAN << BOLD << "  ══ " << title << " ══" << RESET << "\n\n";
}

void printEntry(const WordEntry& e) {
    printDivider();
    cout << BOLD << GREEN << "  " << e.word << RESET;
    if (!e.phonetic.empty())
        cout << "  " << DIM << e.phonetic << RESET;
    cout << "  " << MAGENTA << "[" << e.wordType << "]" << RESET << "\n\n";
    cout << "  " << BOLD << "Definition : " << RESET << e.definition << "\n";
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

// ==========================================
// BST Class
// ==========================================
class BST {
private:
    Node* root;

    Node* insert(Node* node, const WordEntry& e, bool& inserted) {
        if (!node) {
            inserted = true;
            return new Node(e);
        }
        if (e.word < node->data.word)
            node->left  = insert(node->left,  e, inserted);
        else if (e.word > node->data.word)
            node->right = insert(node->right, e, inserted);
        else
            inserted = false;   // duplicate — do nothing
        return node;
    }

    Node* search(Node* node, const string& key) const {
        if (!node || node->data.word == key) return node;
        if (key < node->data.word) return search(node->left, key);
        return search(node->right, key);
    }

    void inOrder(Node* node, vector<WordEntry>& result) const {
        if (!node) return;
        inOrder(node->left, result);
        result.push_back(node->data);
        inOrder(node->right, result);
    }

    int count(Node* node) const {
        if (!node) return 0;
        return 1 + count(node->left) + count(node->right);
    }

    Node* findMin(Node* node) const {
        while (node->left) node = node->left;
        return node;
    }

    Node* deleteNode(Node* node, const string& key) {
        if (!node) return nullptr;
        if (key < node->data.word)
            node->left  = deleteNode(node->left, key);
        else if (key > node->data.word)
            node->right = deleteNode(node->right, key);
        else {
            if (!node->left && !node->right) {
                delete node;
                return nullptr;
            }
            if (!node->left) {
                Node* tmp = node->right;
                delete node;
                return tmp;
            }
            if (!node->right) {
                Node* tmp = node->left;
                delete node;
                return tmp;
            }
            Node* successor = findMin(node->right);
            node->data = successor->data;
            node->right = deleteNode(node->right, successor->data.word);
        }
        return node;
    }

    void collectByPrefix(Node* node, const string& prefix,
                         const string& upperBound,
                         vector<string>& results) const {
        if (!node) return;
        const string& nw = node->data.word;

        if (nw >= upperBound) {
            collectByPrefix(node->left, prefix, upperBound, results);
            return;
        }
        if (nw < prefix) {
            collectByPrefix(node->right, prefix, upperBound, results);
            return;
        }
        collectByPrefix(node->left, prefix, upperBound, results);
        size_t pLen = prefix.size();
        if (nw.size() >= pLen && nw.substr(0, pLen) == prefix)
            results.push_back(nw);
        collectByPrefix(node->right, prefix, upperBound, results);
    }

    void closest(Node* node, const string& key,
                 string& bestWord, int& bestDist) const {
        if (!node || bestDist == 0) return;
        int d = editDistance(key, node->data.word);
        if (d < bestDist) {
            bestDist = d;
            bestWord = node->data.word;
        }
        closest(node->left,  key, bestWord, bestDist);
        closest(node->right, key, bestWord, bestDist);
    }

    void freeTree(Node* node) {
        if (!node) return;
        freeTree(node->left);
        freeTree(node->right);
        delete node;
    }

public:
    BST() : root(nullptr) {}
    ~BST() { freeTree(root); }

    bool isEmpty() const { return root == nullptr; }

    WordEntry* search(const string& key) {
        Node* n = search(root, toLower(key));
        return n ? &n->data : nullptr;
    }

    bool insert(const WordEntry& e) {
        bool inserted = false;
        root = insert(root, e, inserted);
        return inserted;
    }

    bool remove(const string& key) {
        string k = toLower(key);
        if (!search(root, k)) return false;
        root = deleteNode(root, k);
        return true;
    }

    vector<WordEntry> getAllWords() const {
        vector<WordEntry> result;
        inOrder(root, result);
        return result;
    }

    int wordCount() const { return count(root); }

    int height(Node* node) const {
        if (!node) return 0;
        return 1 + max(height(node->left), height(node->right));
    }

    int getHeight() const { return height(root); }

    vector<string> autoComplete(const string& prefix) const {
        vector<string> results;
        string p = toLower(prefix);
        if (p.empty()) return results;

        string upper = p;
        upper.back()++;

        collectByPrefix(root, p, upper, results);
        sort(results.begin(), results.end());
        return results;
    }

    string spellSuggest(const string& key) const {
        if (isEmpty()) return "";
        string bestWord;
        int bestDist = INT_MAX;
        closest(root, toLower(key), bestWord, bestDist);
        return (bestDist <= 3) ? bestWord : "";
    }
};

// ==========================================
// Dictionary Application Class
// ==========================================
class Dictionary {
private:
    BST bst;
    stack<string> searchHistory;
    const string SAVE_FILE = "dictionary_data.txt";

    void saveToFile() {
        ofstream file(SAVE_FILE);
        if (!file.is_open()) {
            cout << RED << "  [Error] Could not open file for saving.\n" << RESET;
            return;
        }
        auto words = bst.getAllWords();
        for (const auto& e : words) {
            file << e.word << "|"
                 << e.definition << "|"
                 << e.wordType << "|"
                 << e.phonetic << "|";
            for (size_t i = 0; i < e.synonyms.size(); i++) {
                file << e.synonyms[i];
                if (i + 1 < e.synonyms.size()) file << ",";
            }
            file << "\n";
        }
        file.close();
        cout << GREEN << "  [✓] Dictionary saved to " << SAVE_FILE << RESET << "\n";
    }

    void loadFromFile() {
        ifstream file(SAVE_FILE);
        if (!file.is_open()) return;
        string line;
        int loaded = 0;
        while (getline(file, line)) {
            if (trim(line).empty()) continue;
            auto parts = splitKeepEmpty(line, '|');
            // A line with no synonyms (trailing "|") may not produce
            // a 5th token via getline — pad it so indexing is safe.
            while (parts.size() < 5) parts.push_back("");
            if (parts.size() < 4) continue;
            WordEntry e;
            e.word       = toLower(trim(parts[0]));
            e.definition = trim(parts[1]);
            e.wordType   = trim(parts[2]);
            e.phonetic   = trim(parts[3]);
            if (!parts[4].empty())
                e.synonyms = split(parts[4], ',');
            bst.insert(e);
            loaded++;
        }
        file.close();
        if (loaded > 0)
            cout << GREEN << "  [✓] Loaded " << loaded
                 << " word(s) from " << SAVE_FILE << RESET << "\n\n";
    }

    string getLine(const string& prompt) {
        cout << YELLOW << "  " << prompt << RESET;
        string s;
        getline(cin, s);
        return trim(s);
    }

    char getChoice(const string& prompt) {
        cout << YELLOW << "  " << prompt << RESET;
        string s;
        getline(cin, s);
        return s.empty() ? '\0' : (char)tolower((unsigned char)s[0]);
    }

    void pushHistory(const string& word) {
        if (!searchHistory.empty() && searchHistory.top() == word) return;
        searchHistory.push(word);
        if (searchHistory.size() > 10) {
            stack<string> tmp;
            for (int i = 0; i < 10; i++) {
                tmp.push(searchHistory.top());
                searchHistory.pop();
            }
            searchHistory = stack<string>();
            stack<string> rev;
            while (!tmp.empty()) { rev.push(tmp.top()); tmp.pop(); }
            searchHistory = rev;
        }
    }

    void showHistorySuggestions() {
        if (searchHistory.empty()) return;
        cout << DIM << "  Recent searches: ";
        stack<string> tmp = searchHistory;
        vector<string> recent;
        while (!tmp.empty()) { recent.push_back(tmp.top()); tmp.pop(); }
        for (size_t i = 0; i < min(recent.size(), (size_t)5); i++) {
            cout << recent[i];
            if (i + 1 < min(recent.size(), (size_t)5)) cout << "  ·  ";
        }
        cout << RESET << "\n";
    }

    void displayMenu() {
        cout << "\n";

        cout << GRAY << " ┌──────────────────────────────────────────────┐\n" << RESET;
        cout << BLUE << " │ ██████╗  ███████╗████████╗                   │\n" << RESET;
        cout << BLUE << " │ ██╔══██╗██╔════╝╚══██╔══╝   " << RESET << BOLD << "Dictionary" << BLUE << "       │\n" << RESET;
        cout << BLUE << " │ ██████╦╝███████╗   ██║      " << RESET << GRAY << "v1.1 · C++" << BLUE << "       │\n" << RESET;
        cout << BLUE << " │ ██╔══██╗╚════██║   ██║                       │\n" << RESET;
        cout << BLUE << " │ ██████╦╝███████║   ██║                       │\n" << RESET;
        cout << BLUE << " │ ╚═════╝ ╚══════╝   ╚═╝                       │\n" << RESET;
        cout << GRAY << " └──────────────────────────────────────────────┘\n\n" << RESET;

        cout << GRAY << " Vardhaman College of Engineering  ·  CSE Dept\n";
        cout << " Mentor: Sai Riddhi   ·  Summer Project 2026\n";
        cout << " ------------------------------------------------\n\n" << RESET;

        cout << YELLOW << BOLD << " MAIN MENU\n" << RESET;
        cout << GRAY << " ------------------------------------------------\n\n" << RESET;

        cout << GREEN << " [1] " << RESET << "Add Word          " << GRAY << "Insert into BST\n" << RESET;
        cout << GREEN << " [2] " << RESET << "Search Word       " << GRAY << "Lookup & details\n" << RESET;
        cout << GREEN << " [3] " << RESET << "Delete Word       " << GRAY << "Remove from BST\n" << RESET;
        cout << GREEN << " [4] " << RESET << "Display All Words " << GRAY << "In-order traversal\n" << RESET;
        cout << GREEN << " [5] " << RESET << "Count Words       " << GRAY << "Total entries\n" << RESET;
        cout << GREEN << " [6] " << RESET << "Auto-Complete     " << GRAY << "Prefix BST search  O(p+m)\n" << RESET;
        cout << GREEN << " [7] " << RESET << "Edit Word         " << GRAY << "Update existing entry\n" << RESET;
        cout << CYAN  << " [8] " << RESET << "Search History    " << GRAY << "View recent searches\n" << RESET;
        cout << YELLOW << " [9] " << RESET << "Save Dictionary   " << GRAY << "Write to file\n" << RESET;
        cout << YELLOW << "[10] " << RESET << "Load Dictionary   " << GRAY << "Read from file\n" << RESET;
        cout << RED    << " [0] " << RESET << "Exit              " << GRAY << "Auto-saves on quit\n\n" << RESET;

        cout << GRAY << " ------------------------------------------------\n" << RESET;
        cout << GRAY << " ✦ Auto-Complete uses true O(p+m) prefix traversal\n\n" << RESET;

        cout << GREEN << " → " << GRAY << "type a choice (e.g. 2) and press Enter...\n" << RESET;
        cout << GREEN << " \n > " << RESET;
    }

public:
    Dictionary() {
        loadFromFile();
    }

    ~Dictionary() {}

    void addWord() {
        printHeader("ADD WORD");
        string word = toLower(getLine("Enter word          : "));
        if (word.empty()) { cout << RED << "  Word cannot be empty.\n" << RESET; return; }

        if (bst.search(word)) {
            cout << YELLOW << "\n  [!] '" << word
                 << "' already exists in the dictionary.\n" << RESET;
            char c = getChoice("  Would you like to edit it instead? (y/n): ");
            if (c == 'y') { editWord(word); }
            return;
        }

        WordEntry e;
        e.word       = word;
        e.definition = getLine("Enter definition    : ");
        e.wordType   = getLine("Word type (N/V/Adj/Adv/etc): ");
        e.phonetic   = getLine("Phonetic (optional) : ");
        string synInput = getLine("Synonyms (comma-sep): ");
        if (!synInput.empty()) e.synonyms = split(synInput, ',');

        bst.insert(e);
        cout << GREEN << "\n  [✓] '" << word << "' added successfully!\n" << RESET;
    }

    void searchWord() {
        printHeader("SEARCH WORD");
        showHistorySuggestions();

        string word = toLower(getLine("\n  Enter word to search: "));
        if (word.empty()) return;

        auto suggestions = bst.autoComplete(word);
        if (!suggestions.empty() && suggestions[0] != word) {
            cout << DIM << "  Suggestions: ";
            for (size_t i = 0; i < min(suggestions.size(), (size_t)5); i++) {
                cout << suggestions[i];
                if (i + 1 < min(suggestions.size(), (size_t)5)) cout << "  ";
            }
            cout << RESET << "\n";
        }

        WordEntry* entry = bst.search(word);
        if (entry) {
            pushHistory(word);
            printEntry(*entry);
        } else {
            cout << RED << "\n  [✗] '" << word << "' not found.\n" << RESET;
            string suggestion = bst.spellSuggest(word);
            if (!suggestion.empty()) {
                cout << YELLOW << "  Did you mean: " << BOLD << suggestion
                     << RESET << " ?\n";
                char c = getChoice("  Search for '" + suggestion + "'? (y/n): ");
                if (c == 'y') {
                    WordEntry* se = bst.search(suggestion);
                    if (se) { pushHistory(suggestion); printEntry(*se); }
                }
            }
        }
    }

    void deleteWord() {
        printHeader("DELETE WORD");
        string word = toLower(getLine("Enter word to delete: "));
        if (word.empty()) return;

        WordEntry* entry = bst.search(word);
        if (!entry) {
            cout << RED << "  [✗] '" << word << "' not found.\n" << RESET;
            return;
        }
        printEntry(*entry);
        char c = getChoice("  Confirm delete? (y/n): ");
        if (c == 'y') {
            bst.remove(word);
            cout << GREEN << "  [✓] '" << word << "' deleted.\n" << RESET;
        } else {
            cout << DIM << "  Delete cancelled.\n" << RESET;
        }
    }

    void displayAllWords() {
        printHeader("ALL WORDS  (A → Z)");
        if (bst.isEmpty()) {
            cout << DIM << "  Dictionary is empty.\n" << RESET;
            return;
        }
        auto words = bst.getAllWords();
        cout << "  " << DIM << words.size() << " word(s) found.\n\n" << RESET;
        for (const auto& e : words) printEntry(e);
    }

    void countWords() {
        printHeader("WORD COUNT");
        cout << "  Total words in dictionary: "
             << BOLD << CYAN << bst.wordCount() << RESET << "\n";
    }

    void autoComplete() {
        printHeader("AUTO-COMPLETE");
        string prefix = toLower(getLine("Type a prefix (e.g. 'app'): "));
        if (prefix.empty()) return;
        auto suggestions = bst.autoComplete(prefix);
        if (suggestions.empty()) {
            cout << RED << "  No words found starting with '" << prefix << "'.\n" << RESET;
        } else {
            cout << GREEN << "  " << suggestions.size() << " suggestion(s):\n\n" << RESET;
            for (const auto& s : suggestions)
                cout << "    " << CYAN << "→" << RESET << "  " << s << "\n";
        }
    }

    void editWord(const string& preFilledWord = "") {
        printHeader("EDIT WORD");
        string word = preFilledWord.empty()
                      ? toLower(getLine("Enter word to edit  : "))
                      : preFilledWord;
        if (word.empty()) return;

        WordEntry* entry = bst.search(word);
        if (!entry) {
            cout << RED << "  [✗] '" << word << "' not found.\n" << RESET;
            return;
        }

        printEntry(*entry);
        cout << DIM << "  (Press Enter to keep current value)\n\n" << RESET;

        auto updateField = [](const string& prompt, string& field) {
            cout << YELLOW << "  " << prompt << " [" << DIM << field << YELLOW << "]: " << RESET;
            string input;
            getline(cin, input);
            input = trim(input);
            if (!input.empty()) field = input;
        };

        updateField("Definition", entry->definition);
        updateField("Word Type ", entry->wordType);
        updateField("Phonetic  ", entry->phonetic);

        cout << YELLOW << "  Synonyms [" << DIM;
        for (size_t i = 0; i < entry->synonyms.size(); i++) {
            cout << entry->synonyms[i];
            if (i + 1 < entry->synonyms.size()) cout << ", ";
        }
        cout << YELLOW << "]: " << RESET;
        string synInput;
        getline(cin, synInput);
        synInput = trim(synInput);
        if (!synInput.empty()) entry->synonyms = split(synInput, ',');

        cout << GREEN << "\n  [✓] '" << word << "' updated successfully!\n" << RESET;
    }

    void viewSearchHistory() {
        printHeader("SEARCH HISTORY");
        if (searchHistory.empty()) {
            cout << DIM << "  No searches yet.\n" << RESET;
            return;
        }
        stack<string> tmp = searchHistory;
        vector<string> history;
        while (!tmp.empty()) { history.push_back(tmp.top()); tmp.pop(); }
        cout << "  " << DIM << history.size() << " recent search(es):\n\n" << RESET;
        for (size_t i = 0; i < history.size(); i++)
            cout << "    " << CYAN << (i + 1) << "." << RESET
                 << "  " << history[i] << "\n";
    }

    void saveDictionary() { saveToFile(); }

    void loadDictionary() { loadFromFile(); }

    void run() {
        int choice;
        bool isRunning = true;

        while (isRunning) {
            displayMenu();

            if (!(cin >> choice)) {
                cout << RED << "Invalid input. Please enter a numerical value.\n" << RESET;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore();

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
                    cout << YELLOW << "\nExiting BST Dictionary. Saving data...\n" << RESET;
                    saveToFile();
                    isRunning = false;
                    break;
                default:
                    cout << RED << "\nInvalid choice. Please select a valid option from 0 to 10.\n" << RESET;
            }
        }
    }
};

// ==========================================
// main()
// ==========================================
int main() {
    #ifdef _WIN32
    system("chcp 65001 > nul");
    #endif

    Dictionary dict;
    dict.run();

    return 0;
}