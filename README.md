# SecureBank C++ Backend System

An enterprise-grade terminal banking application built in C++. This project demonstrates secure relational database management, memory-safe architecture, and the Single Responsibility Principle by decoupling the user interface from the core banking logic.

## 🚀 Key Features

* **Secure Relational Database:** Utilizes a 3-table SQLite schema (Users, Vault, Transactions) with cascading foreign keys and strictly parameterized C-API queries to prevent SQL injection.
* **Memory-Safe Architecture:** Implements a strict `std::unique_ptr` ownership model in the RAM vault to ensure zero memory leaks during operation.
* **Cryptographic Security:** Features salted password hashing for all user accounts.
* **Clean Architecture:** Separates the terminal UI (`BankUI`) from the backend API (`BankManager`) using object-oriented principles.
* **Automated Builds:** Managed via a custom `Makefile` for instant compilation.
* **Ledger Export:** Dynamic CSV file generation utilizing C++ `<fstream>` for real-world transaction logging.

## 🧠 System Architecture

The application is split into two isolated layers:
1. **The Brain (`BankManager`):** A silent backend API that exclusively handles SQLite execution, memory maps, and financial mathematics (compound interest, time-deltas).
2. **The Face (`BankUI`):** The frontend terminal interface that manages `cin/cout` operations and sanitizes user input before passing it to the Manager via pointer reference.

## 🗄️ Database Schema

The SQLite database (`secure_vault.db`) enforces relational integrity across three tables:
* `Users`: Stores primary account demographics, salted hashes, and `Account_Number` (Primary Key).
* `Vault`: Stores financial balances and account types (Savings vs. Checking), linked via Foreign Key.
* `Transactions`: An append-only ledger tracking deposits, withdrawals, transfers, and overdraft penalties.

## 🛠️ Prerequisites

To compile and run this project, you will need:
* A C++ compiler (e.g., `g++` via MinGW for Windows or GCC for Linux)
* `make` (or `mingw32-make` on Windows)
* SQLite3 (Pre-compiled `sqlite3.o` and `sqlite3.h` included in the source)

## ⚙️ Build and Run Instructions

This project uses a Makefile for automated compilation.

1. Clone the repository:
   ```bash
   git clone [https://github.com/](https://github.com/)[YourUsername]/[YourRepositoryName].git
   cd [YourRepositoryName]
2. Build the executable:
   make
3. Run the server:
   ./SecureBank.exe
Author : [Ankit] - Software Engineering & Architecture - [(https://github.com/Ankityadav516)]
