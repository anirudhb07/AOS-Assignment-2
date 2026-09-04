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


## Module Overview

src/main.cpp: Entry point handling the REPL loop, signal setup (SIGINT, SIGTSTP, SIGCHLD), and graceful shutdown.

src/helpers.cpp: String manipulation, path mapping for ~, whitespace trimming, and dynamic memory tokenization.

src/prompt.cpp: Renders the shell prompt format <username@system_name:relative_path>.

src/builtins.cpp: Custom implementations for cd, pwd, echo, ls (-a, -l), and search.

src/pinfo.cpp: Parses /proc/<pid> files (status, statm, exe) to display process details.

src/history.cpp: Manages session history ring buffer and .shell_history persistence.

src/execute.cpp: Manages command pipeline chains (|), I/O redirection (<, >, >>), built-in dispatching, and background process spawning (&).

src/readline_custom.cpp: Custom POSIX terminal driver handling raw mode, TAB autocomplete, cursor navigation, and history navigation via arrow keys.