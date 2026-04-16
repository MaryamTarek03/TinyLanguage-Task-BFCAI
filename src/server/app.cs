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
    if (libraryName == "liblexer.so")
        return NativeLibrary.Load(Path.Combine(currentDir, libraryName));
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

app.Run();

// --- P/Invoke Definitions ---

// Mirror the C++ enum exactly so we can translate the integers back to strings
public enum TokenType {
    IF_KW, THEN_KW, ELSE_KW, END_KW, REPEAT_KW, UNTIL_KW, READ_KW, WRITE_KW,
    ID, NUMBER,
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
    private const string LibName = "liblexer.so"; 

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern LexResult Tokenize([MarshalAs(UnmanagedType.LPUTF8Str)] string sourceCode);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FreeResult(LexResult result);
}