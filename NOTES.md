# Notes Taken During Development



## Building the Program for Debugging

```
gcc -std=c11 -Wall -Wextra -pedantic -Og -g .\src\main.c .\src\source.c .\src\scanner.c .\src\parser.c .\src\emitter.c -o .\build\msl.exe
```



## CLI Functionality

### Help

```powershell
.\build\msl.exe --help
```

### Token Dump

```powershell
.\build\msl.exe .\examples\hello_sequence.msl --tokens
```

### Compile to JSON

```powershell
.\build\msl.exe .\examples\hello_sequence.msl -o .\build\hello_sequence.msl.json
```



## Open-Ended Notes

### Unconfirmed Scanner Choices

* Leading zeros are rejected (`0123`), following the JSON number format ([src/scanner.c:148](src/scanner.c#L148))
* Timestamps are checked for format only; value checks would go in the parser ([src/scanner.c:316](src/scanner.c#L316))
* Relative timestamps max out at 99 hours; longer waits would need a day field ([src/scanner.c:317](src/scanner.c#L317))
* Raw tabs inside strings are accepted, but JSON forbids them; either the JSON writer escapes them or the scanner rejects them ([src/scanner.c:223](src/scanner.c#L223))
* Non-ASCII bytes outside strings give one error per byte, so one UTF-8 character can produce several errors ([src/scanner.c:403](src/scanner.c#L403))

### Scanner Gaps

* A NUL byte in the file silently ends scanning; `is_eof()` treats `'\0'` as the end of input, but `read_source()` reads in binary mode and can include NUL bytes ([src/scanner.c:24](src/scanner.c#L24), [src/source.c:13](src/source.c#L13))
* No function to free a `TokenList` callers must `free(list.tokens)` themselves ([src/scanner.h:15](src/scanner.h#L15))

### TODO and TEMPORARY Comments

* None
