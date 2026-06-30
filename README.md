# BST-Based Dictionary

### A Production-Style Dictionary Application Powered by Binary Search Trees in C++

Developed as part of the **Summer Project 2025–26** at **Vardhaman College of Engineering (VCEH), CSE Department**.

---

## Team

### Team Lead

* Rendla Vishnu Tej

### Members

* Araveti Harika
* Paspula Akshith
* Nagulapally Manideep
* Sakinala Sanjitha

### Mentor

* Vechha Sai Riddhi

---

## Project Overview

BST-Based Dictionary is a feature-rich console application built entirely in C++ that demonstrates how a classical data structure can be transformed into a practical software system.

The application uses a custom implementation of a **Binary Search Tree (BST)** to efficiently manage dictionary entries while integrating additional features such as auto-complete, spell suggestions, search history, and persistent storage.

The goal of the project was to move beyond textbook implementations and develop a production-style application using core Data Structures and Algorithms concepts.

---

## Features

### Dictionary Operations

* Add new words with:

  * Definition
  * Word Type
  * Phonetic Representation
  * Synonyms
* Search for words
* Edit existing entries
* Delete words from the dictionary
* Display all words in alphabetical order
* Count total dictionary entries

### Advanced Features

* Prefix-based Auto-Complete Suggestions
* Edit Distance based Spell Suggestions
* Duplicate Entry Detection
* Search History Tracking using Stack
* Persistent File Storage
* Automatic Data Loading on Startup
* Automatic Data Saving on Exit
* ANSI Styled Command Line Interface

---

## Data Structures Used

### Binary Search Tree (BST)

Used for:

* Insertion
* Searching
* Deletion
* Alphabetical Traversal
* Prefix Search

### Stack

Used for:

* Maintaining Recent Search History

---

## Algorithms Implemented

* Binary Search Tree Insertion
* Binary Search Tree Search
* Binary Search Tree Deletion
* In-order Traversal
* Prefix-based Traversal for Auto-Complete
* Edit Distance Algorithm for Spell Suggestions
* Duplicate Detection Logic
* Persistent File Storage using File Handling

---

## BST Deletion Cases Supported

The application correctly handles all BST deletion scenarios:

* Leaf Node Deletion
* Single Child Deletion
* Two Child Deletion using In-order Successor

---

## Repository Structure

```text
BST-based-Dictionary/
│
├── Bst_core.cpp
├── dictionary_data.txt
├── README.md
├── UML_Diagram.jpeg
└── Structural_Data_Model.jpeg
```

---

## Compilation

### Requirements

* GCC Compiler
* g++ with C++17 support
* Visual Studio Code / CodeBlocks / Any C++ IDE

### Compile

```bash
g++ -std=c++17 -o dictionary Bst_core.cpp
```

### Run

#### Linux / macOS

```bash
./dictionary
```

#### Windows

```bash
dictionary.exe
```

---

## Application Workflow

1. The application starts.
2. Existing dictionary entries are automatically loaded from file.
3. The user selects operations through a menu-driven interface.
4. All operations are performed directly on the BST stored in memory.
5. Before termination, the dictionary is automatically saved.
6. During subsequent launches, all previously stored data is restored automatically.

---

## Sample Features Demonstrated

### Auto Complete

```text
Input:
pro

Output:
procrastination
progress
programming
project
```

### Spell Suggestion

```text
Input:
procastination

Output:
Did you mean: procrastination?
```

### Persistent Storage

```text
Loaded 50 word(s) from dictionary_data.txt
```

---

## Concepts Demonstrated

* Data Structures
* Algorithms
* Object-Oriented Programming
* Recursion
* File Handling
* Dynamic Memory Allocation
* String Manipulation
* STL Containers
* Console UI Design

---

## Future Improvements

* AVL Tree Implementation for guaranteed O(log n) operations
* Trie-based Auto-Complete System
* GUI Version using Qt
* Multi-language Dictionary Support
* Audio Pronunciation Support
* Cloud Synchronization
* Export Dictionary to PDF

---

## Applications

* Vocabulary Learning
* Offline Dictionary System
* DSA Demonstration Project
* Educational Tool
* File Handling Demonstration
* BST Visualization and Learning

---

## License

This repository is maintained for educational and academic purposes as part of the Summer Project initiative at Vardhaman College of Engineering during the academic year 2025–26.
