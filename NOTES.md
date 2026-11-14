# Notes Taken During Development



## Building the Program

```
gcc -std=c11 -Wall -Wextra -pedantic .\src\main.c .\src\source.c -o .\build\msl.exe
```



## CLI Functionality

### Basic Read

```powershell
.\build\msl.exe .\examples\hello_sequence.msl
```

### Output Path

```powershell
.\build\msl.exe .\examples\hello_sequence.msl -o .\examples\hello_sequence.msl.json
```
