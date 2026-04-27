# 🔄 CodeMorpher — C to Python Translator

## 🔗 Project Overview
CodeMorpher is a source-to-source translator that automatically
converts C programs into equivalent Python programs using Lex
and Yacc tools of compiler design.
It helps students understand how the same logic works in both
C and Python by translating C code to Python while preserving
the original functionality.

---

## ⚡ Project Description
Students learning programming often struggle with C language
due to its complex syntax, memory management and low level
structure. They later move to Python for its simplicity.
Manually rewriting C code to Python is time consuming and
error prone. CodeMorpher solves this problem by:

- Automatically converting C source code to Python using
  Lexical Analysis and Syntax Analysis.
- Using Lex to tokenize C code into keywords, operators,
  identifiers and literals.
- Using Yacc to parse the token stream and generate
  equivalent Python code.
- Preserving the original logic and functionality of the
  C program in the translated Python output.

---

## ⚙️🔍 Features
- Arithmetic operations
- if / else if / else conditions
- while loop
- for loop
- Single line and multi line comments
- Functions
- printf and scanf
- break / continue / return
- Logical and relational operators

---

## 🖥️ Prerequisites

- WSL (Ubuntu)
- Python 3
- make
- pip
- flex (Lex)
- bison (Yacc)


---

## 🚀 How to Use

1. Install required tools
```bash
sudo apt install python3 python3-pip python3-venv make flex bison gcc -y
```

2. Install Python dependencies
```bash
pip install -r requirements.txt
```

3. Run the translator
```bash
make
```

4. Run app.py
```bash
python app.py
```

---

## ✅ Assumptions 💡
- Input C program is syntactically correct and error free
- No complex data structures like pointers or structs
- No logical errors in input program

---

## 🔮 Future Enhancements
- Add support for pointers and structures
- Support arrays and string operations
- Add a graphical user interface for easier translation
- Generate optimized intermediate code
- Generate more optimized and readable Python code

---

