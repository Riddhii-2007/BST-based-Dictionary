# BST-Based Dictionary

### A Console Application powered by Binary Search Tree | C++

**Vardhaman College of Engineering — CSE Department — Summer Project 2025–26**

---

# Team

## Team Lead
- Rendla Vishnu Tej

## Members
- Araveti Harika
- Paspula Akshith
- Nagulapally Manideep
- Sakinala Sanjitha

## Mentor
- Vechha Sai Riddhi

---

# What is this?

A fully functional dictionary application that runs in the terminal.

All word storage and retrieval is powered by a self-built Binary Search Tree (BST) implemented in C++.

The project demonstrates practical usage of Data Structures and File Handling concepts through a real-world console application.

---

# Features

1. Add Word — with definition, word type, synonyms, and phonetic
2. Search Word — displays complete word details
3. Delete Word — supports all 3 BST deletion cases
4. Display All Words — alphabetical order using BST in-order traversal
5. Count Total Words — shows total entries in dictionary
6. Word Type — noun, verb, adjective, adverb, etc.
7. Synonyms — related words shown during search
8. Search History — recent searches stored using Stack
9. Auto-Complete — instant suggestions using word prefix
10. Save / Load — automatic file saving and loading
11. Edit Word — modify definition, type, synonyms, or phonetic
12. Duplicate Alert — warns if word already exists
13. Spell Suggestion — suggests closest matching word

---

# Data Structures Used

- Binary Search Tree (BST) — core storage and retrieval
- Stack — search history management

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

# How to Compile & Run

## Requirements
- C++ Compiler (GCC / Code::Blocks / VS Code)

## Compile

```bash
g++ -o dictionary main.cpp BST.cpp Dictionary.cpp
```

## Run

### Linux / Mac

```bash
./dictionary
```

### Windows

```bash
dictionary.exe
```

---

# How the Application Works

1. The application starts and automatically loads saved dictionary data from file.
2. User selects an operation from the menu.
3. All operations are performed on the BST stored in memory.
4. Before exiting, the dictionary is automatically saved to file.
5. During the next launch, all previous data is restored.

---

# Concepts Used

- Binary Search Tree Operations
  - Insertion
  - Searching
  - Deletion
  - In-order Traversal

- Stack Operations
  - Push
  - Pop
  - Display History

- File Handling
  - Read from file
  - Write to file

- String Manipulation
- Menu Driven Programming
- Object-Oriented Programming in C++

---

# Future Improvements

- GUI version using Qt or Java Swing
- User authentication system
- Multiple dictionary support
- Advanced spell checker
- Word pronunciation audio
- Export dictionary to PDF

---

# Sample Use Cases

- Student vocabulary learning
- Mini offline dictionary
- DSA project demonstration
- BST operation visualization
- File handling practice project

---

# License

Academic Project — Vardhaman College of Engineering (2025–26)
