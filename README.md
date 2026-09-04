# POSIX Shell Implementation

## Assignment 2 - Advanced Operating Systems
**Language:** C++ (POSIX C APIs)  

---

## Overview
This project implements a modular POSIX-compliant interactive shell in C++ using Unix system calls (`fork`, `execvp`, `pipe`, `dup2`, `waitpid`, `termios`). It features line parsing, built-in commands, external system execution, background job tracking, process monitoring via `/proc`, recursive directory searching, line editing with auto-completion, and command history persistence across sessions.

---

## Directory Structure

```text
2026201058_Assignment2/
├── Makefile
├── README.md
├── header.h
└── src/
    ├── main.cpp
    ├── helpers.cpp
    ├── prompt.cpp
    ├── builtins.cpp
    ├── pinfo.cpp
    ├── history.cpp
    ├── execute.cpp
    └── readline_custom.cpp
```

## Build and Run

1. Navigate to the project directory
```bash
cd 2026201058_Assignment2
```
2. Compile the project

Use the provided Makefile:
```bash
make
```

3. Run the shell

After successful compilation, run:
```bash
./shell
```

4. Clean compiled files

To remove the generated executable and object files:
```bash
make clean
```