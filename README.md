# Tiny Language Lexer

This is a lexical analyzer (lexer) for a "Tiny Language." It is built using a C++ core for fast tokenization, a C# (.NET) backend server that provides a REST API, and a vanilla HTML/CSS/JS frontend that features customizable themes and syntax highlighting.

## Requirements

To build and run this project, you will need the following installed on your system:

- **g++ (GCC)**: To compile the C++ lexer into a shared library.
- **.NET SDK**: To run the C# backend server (compatible with .NET 10+).
- **Web Browser**: A modern web browser to view the frontend interface.

## How to Run

Follow these steps to get the application running locally:

### 1. Compile the C++ Lexer

You first need to compile the C++ logic into a shared library (`liblexer.so`) that the C# backend can invoke. Open a terminal in the root of the project and run:

```bash
g++ -shared -fPIC -o "src/server/liblexer.so" "src/backend/lexer.cpp"
```

### 2. Start the Backend Server

Next, start the C# backend server, which hosts the `/api/tokenize` endpoint. From the root of the project, run:

```bash
dotnet run "src/server/app.cs"
```

_Note: Make sure the server indicates it is running and listening on `http://localhost:5000`._

### 3. Open the Frontend

Finally, open the `src/frontend/index.html` file in your preferred web browser.

You can simply double-click the file in your file explorer, or serve it using an extension like "Live Server" in VS Code. Once open, you can type Tiny Language code into the text area, click "Run", and see the syntax highlighting and token table update!
