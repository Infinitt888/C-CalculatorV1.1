# C Calculator
 
A lightweight calculator written in **pure C** with no external dependencies.
Features a command-line version that runs on Windows, Linux, and macOS, and a
native Windows GUI version built with WinAPI.
 
---
 
## Features
 
- Hand-written recursive-descent parser with correct operator precedence
- Supports `+`, `-`, `*`, `/`, `%` (modulo), `^` (power), and `()` parentheses
- Floating-point and integer arithmetic
- Expression history (terminal version)
- Native Windows GUI with keyboard and mouse input
- Clean, consistent C code style throughout
---
 
## Versions
 
| Version | File | Platform |
|---|---|---|
| Terminal | `calculator.c` | Windows, Linux, macOS |
| GUI | `calculator_gui.c` | Windows only (WinAPI) |
 
---
 
## Getting Started
 
### Terminal Version
 
**Linux / macOS**
```bash
gcc -o calculator calculator.c -lm
./calculator
```
 
**Windows (MinGW)**
```bash
gcc -o calculator calculator.c -lm
calculator.exe
```
 
**Windows (Visual Studio)**
1. Create a new **Empty Project**
2. Add `calculator.c` to Source Files (keep the `.c` extension)
3. Press `Ctrl+Shift+B` to build
4. Press `Ctrl+F5` to run
### GUI Version (Windows only)
 
1. Create a new **Empty Project** in Visual Studio
2. Add `calculator_gui.c` to Source Files
3. Go to **Project Properties → Linker → System**
4. Set **SubSystem** to `Windows (/SUBSYSTEM:WINDOWS)`
5. Go to **Linker → Input → Additional Dependencies**
6. Add `comctl32.lib`
7. Press `Ctrl+Shift+B` to build
---
 
## Usage
 
### Terminal
Type any expression at the `>` prompt and press Enter:
 
```
> 2 + 2
  = 4
> (3 + 4) * 2
  = 14
> 2 ^ 8
  = 256
> 10 % 3
  = 1
> history
> clear
> quit
```
 
### GUI
- Click the buttons or type directly with your keyboard
- Press `Enter` to evaluate
- Press `Escape` to clear
- Press `Backspace` to delete
---
 
## Operator Precedence
 
From lowest to highest:
 
| Precedence | Operators |
|---|---|
| 1 (lowest) | `+` `-` |
| 2 | `*` `/` `%` |
| 3 | `^` (right-associative) |
| 4 | unary `-` |
| 5 (highest) | `( )` |
 
---
 
## Project Structure
 
```
c-calculator/
├── calculator.c        # Terminal version (cross-platform)
├── calculator_gui.c    # Windows GUI version (WinAPI)
└── README.md
```
 
---
 
## License
 
MIT License — see [LICENSE](LICENSE) for details.
