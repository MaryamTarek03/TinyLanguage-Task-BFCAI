#:sdk Microsoft.NET.Sdk.Web
#:property PublishAot=false

using System.Runtime.InteropServices;
using System.Reflection;

string currentDir = Path.GetDirectoryName(Path.GetFullPath("app.cs")) ?? "";
Console.WriteLine($"Current directory: {currentDir}");

// For the liblexer.so to be found, we need to tell .NET exactly where to look for it
// Single file apps don't have a "base directory" like traditional .NET apps, so we use the assembly location
NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), (libraryName, assembly, searchPath) =>
{
    if (libraryName == "liblexer")
    {
        // Detect the operating system
        string extension = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? ".dll" : ".so";
        string fullPath = Path.Combine(currentDir, libraryName + extension);
        
        Console.WriteLine($"Attempting to load native library from: {fullPath}");
        return NativeLibrary.Load(fullPath);
    }
    return IntPtr.Zero;
});

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddCors(options => {
    options.AddDefaultPolicy(policy => policy.AllowAnyOrigin().AllowAnyHeader().AllowAnyMethod());
});

var app = builder.Build();
app.UseCors();

app.MapPost("/api/tokenize", async (HttpContext context) =>
{
    using var reader = new StreamReader(context.Request.Body);
    var sourceCode = await reader.ReadToEndAsync();

    var result = Lexer.Tokenize(sourceCode);
    
    var outputData = new List<object[]>(result.count);
    int structSize = Marshal.SizeOf<CToken>();
    
    for (int i = 0; i < result.count; i++)
    {
        var ptr = IntPtr.Add(result.tokens, i * structSize);
        var token = Marshal.PtrToStructure<CToken>(ptr);
        
        string lexeme = Marshal.PtrToStringUTF8(token.lexeme) ?? "";
        
        string tokenType = ((TokenType)token.type).ToString(); 

        outputData.Add(new object[] { token.line, tokenType, lexeme , token.startIndex, token.length });
    }

    Lexer.FreeResult(result);

    return Results.Json(outputData);
});

app.MapPost("/api/parse", async (HttpContext context) =>
{
    // Try to read useTerminators from query string, defaulting to false if not provided
    bool useTerminators = context.Request.Query["useTerminators"] == "true";

    using var reader = new StreamReader(context.Request.Body);
    string code = await reader.ReadToEndAsync();

    // Call the C++ Parser engine
    CParseResult result = NativeLexer.ParseCode(code, useTerminators);

    var errorList = new List<object>();

    if (result.errorCount > 0 && result.errors != IntPtr.Zero)
    {
        // Calculate the size of one struct so we can jump through memory
        int structSize = Marshal.SizeOf(typeof(CSyntaxError));

        for (int i = 0; i < result.errorCount; i++)
        {
            // Jump to the correct memory address for this specific error
            IntPtr currentPtr = IntPtr.Add(result.errors, i * structSize);
            CSyntaxError error = Marshal.PtrToStructure<CSyntaxError>(currentPtr);

            string message = Marshal.PtrToStringUTF8(error.message) ?? "Unknown Error";

            errorList.Add(new { 
                line = error.line, 
                charIndex = error.charIndex, 
                message = message 
            });
        }
    }

    // Clean up the C++ RAM!
    NativeLexer.FreeParseResult(result);

    // Return the JSON list to the web frontend
    return Results.Json(errorList);
});

app.Run();


// --- 1. Lexer Types ---
public enum TokenType {
    IF_KW, THEN_KW, ELSE_KW, END_KW, REPEAT_KW, UNTIL_KW, READ_KW, WRITE_KW,
    ID, NUMBER, STRING,
    ADDOP, SUBOP, MULOP, DIVOP,
    COMPARISONOP, ASSIGNMENTOP,
    SEMICOLON, PUNCTUATION, COMMA,
    ENDFILE, UNKNOWN
}

[StructLayout(LayoutKind.Sequential)]
public struct CToken
{
    public int type;
    public IntPtr lexeme;
    public int line;
    public int startIndex;
    public int length;
}

[StructLayout(LayoutKind.Sequential)]
public struct LexResult
{
    public IntPtr tokens;
    public int count;
}

public static class Lexer
{
    private const string LibName = "liblexer"; 

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern LexResult Tokenize([MarshalAs(UnmanagedType.LPUTF8Str)] string sourceCode);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FreeResult(LexResult result);
}

// --- 2. Parser Types ---
[StructLayout(LayoutKind.Sequential)]
public struct CSyntaxError
{
    public int line;
    public int charIndex;
    public IntPtr message;
}

[StructLayout(LayoutKind.Sequential)]
public struct CParseResult
{
    public IntPtr errors; 
    public int errorCount;
}

// Wrapping your parser methods in their own static class!
public static class NativeLexer
{
    private const string LibName = "liblexer";

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern CParseResult ParseCode([MarshalAs(UnmanagedType.LPUTF8Str)] string sourceCode, bool useTerminators = true);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FreeParseResult(CParseResult result);
}