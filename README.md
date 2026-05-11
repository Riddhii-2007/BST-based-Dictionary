# BST-based-Dictionary
# BST-Based Dictionary

### A Console Application powered by Binary Search Tree | C++

**Vardhaman College of Engineering — CSE Department — Summer Project 2025–26**

## Team
- **Team Lead:** Rendla Vishnu Tej
- Araveti Harika
- Paspula Akshith
- Nagulapally Manideep
- Sakinala Sanjitha

## Mentor
- Vechha Sai Riddhi

---

# What is this?

A fully functional dictionary application that runs in the terminal.

All word storage and retrieval is powered by a self-built Binary Search Tree (BST) in C++.

---

# How to Compile & Run

## Requirements
- C++ Compiler (GCC / Code::Blocks / VS Code)

## Compile

```bash
g++ -o dictionary main.cpp
```

## Run

```bash
./dictionary
```

---

# Features

1. Add Word — with definition, word type, synonyms, phonetic  
2. Search Word — displays full word details  
3. Delete Word — BST deletion (all 3 cases)  
4. Display All Words — alphabetical order via BST in-order traversal  
5. Count Total Words — total entries in dictionary  
6. Word Type — Noun, Verb, Adjective, Adverb etc.  
7. Synonyms — related words shown on every search  
8. Search History — recent searches via Stack  
9. Auto-Complete — type first few letters, get instant suggestions  
10. Save / Load — dictionary saves to file on exit, loads on startup  
11. Edit Word — fix definition, type, synonyms, or phonetic  
12. Duplicate Alert — warns if word already exists and offers edit  
13. Spell Suggestion — suggests closest match if word not found  

---

# Data Structures Used

- Binary Search Tree (BST) — core storage and retrieval
- Stack — search history

---

# File Structure

```text
dictionary/
├── main.cpp
├── BST.h
├── BST.cpp
├── Dictionary.h
├── Dictionary.cpp
├── dictionary.txt
└── README.md
```

---

# How the App Works

1. App starts and loads dictionary data from file
2. User selects an option from the menu
3. Operations are performed on BST in memory
4. On exit, data is automatically saved
5. Next session restores previous dictionary

---

# Why Delete in a Dictionary?

A user may accidentally enter an incorrect word (example: `uglyy` instead of `ugly`).

Edit can only modify details inside an entry, not the actual word itself.

Delete allows removal of incorrect entries and supports all 3 BST deletion cases, making it an important DSA implementation feature.

---

# License

Academic Project — Vardhaman College of Engineering (2025–26)
