# Kong Fu Chess

A C++ project scaffolded for development in Cursor / VS Code.

## Prerequisites

Installed on this machine:

- **CMake** 4.3.4
- **LLVM-MinGW** (clang++) via winget
- **clangd** extension (`llvm-vs-code-extensions.vscode-clangd`) — Cursor does not carry `ms-vscode.cpptools` in its marketplace, so clangd provides IntelliSense instead

## Build & Run

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
.\build\kong_fu_chess.exe
```

Or press `Ctrl+Shift+B` to build using the configured task.

## Project Structure

```
kong fu chess/
├── CMakeLists.txt      # Build configuration
├── src/
│   └── main.cpp        # Entry point
└── .vscode/            # Editor tasks, launch, and IntelliSense
```
