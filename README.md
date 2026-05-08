# Tiny Language Compiler (Lexer & Parser)

This is a lexical analyzer (lexer) and parser for a "Tiny Language." It is built using a C++ core for fast tokenization and parsing, a C# (.NET) backend server that provides a REST API, and a vanilla HTML/CSS/JS frontend that features customizable themes, syntax highlighting, and error reporting.

## Requirements

To build and run this project, you will need the following installed on your system:

- **g++ (GCC) or MSVC (Visual Studio)**: To compile the C++ core into a shared library.
- **.NET SDK**: To run the C# backend server (compatible with .NET 10+).
- **Web Browser**: A modern web browser to view the frontend interface.

## How to Run

Follow these steps to get the application running locally:

### 1. Clone the Repository

First, clone the repository to your local machine and navigate into the folder:

```bash
git clone https://github.com/MaryamTarek03/TinyLanguage-Task-BFCAI.git
cd TinyLanguage-Task-BFCAI
```

### 2. Compile the C++ Backend

You first need to compile the C++ logic into a shared library (`liblexer.so` or `liblexer.dll`) that the C# backend can invoke. Open a terminal in the root of the project and run the appropriate command for your OS:

**Linux / macOS:**

```bash
g++ -shared -fPIC -o "src/server/liblexer.so" "src/backend/parser.cpp"
```

**Windows (because our hate for Windows won't make us hate our friends!~ 💙):**

_Using GCC/MinGW:_

```powershell
g++ -shared -o "src/server/liblexer.dll" "src/backend/parser.cpp"
```

_Using Visual Studio (MSVC):_
Open the "Developer Command Prompt for VS" and run:

```cmd
cl /LD src\backend\parser.cpp /Fesrc\server\liblexer.dll
```

### 3. Start the Backend Server

Next, start the C# backend server, which hosts the `/api/tokenize` and `/api/parse` endpoints. From the root of the project, run:

```bash
dotnet run "src/server/app.cs"
```

_Note: Make sure the server indicates it is running and listening on `http://localhost:5000`._

### 4. Open the Frontend

Finally, open the `src/frontend/index.html` file in your preferred web browser.

You can simply double-click the file in your file explorer, or serve it using an extension like "Live Server" in VS Code. Once open, you can type Tiny Language code into the text area, click "Run", and see the syntax highlighting and token table update!

## Troubleshooting & Manual Testing

If you encounter issues with the C# web API or want to test the C++ compiler engine in isolation, you can compile and run the core executable directly. The `src/backend/main.cpp` file contains a standalone test runner that executes the lexer and parser.

**Linux / macOS:**

```bash
g++ -o tiny_compiler src/backend/main.cpp
./tiny_compiler
```

**Windows:**

_Using GCC/MinGW:_

```powershell
g++ -o tiny_compiler.exe src/backend/main.cpp
.\tiny_compiler.exe
```

_Using Visual Studio (MSVC):_
Open the "Developer Command Prompt for VS" and run:

```cmd
cl src\backend\main.cpp /Fetiny_compiler.exe
tiny_compiler.exe
```

_Tip: You can modify the `sourceCode` variable inside `src/backend/main.cpp` to test different Tiny Language inputs and toggle settings without spinning up the UI._

## Project Structure

A quick overview of the codebase to help you navigate:

```text
docs/         # Architecture drawings (DFA/NFA)
src/
 ├─ backend/  # C++ Core engine (lexer.cpp, parser.cpp, main.cpp)
 ├─ server/   # C# Web API bridging the browser to the C++ core
 └─ frontend/ # The HTML/CSS/JS Vanilla user interface
```
