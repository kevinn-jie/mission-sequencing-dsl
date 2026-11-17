# Mission Sequencing Domain-Specific Language (DSL) Architecture



<br>



## Table of Contents

1. [Overview](#overview)
2. [Compilation Flow](#compilation-flow)
3. [Tokens](#tokens)
4. [Grammar](#grammar)
5. [Error Checks](#error-checks)
6. [JSON Output](#json-output)
7. [Usage and Workflow](#usage-and-workflow)



<br>



## Overview

Mission Sequencing Language (MSL) was made to be simplistic, human-readable, and more efficient to type than JSON. A source file is a list of metadata headers and timed steps:

```text
@METADATA [key] [value]
[time frame] [timestamp] [stem] [arguments]
```

MSL files (`.msl`) compile to MSL JSON files (`.msl.json`):

```json
{
  "metadata": {
    ...
  },
  "steps": [
    {
      "stem": "...",
      "time": {
        "type": "...",
        "tag": "..."
      },
      "args": [
        ...
      ]
    },
    ...
  ]
}
```

MSL only describes a sequence. It has no variables, expressions, branching, or loops, so a file always compiles to the same output.



<br>



## Compilation Flow

```text
.msl file -> source -> scanner -> parser -> emitter -> .msl.json file
             (text)    (tokens) (sequence)  (JSON)
```

| Stage   | File                           | Takes     | Produces                         | On failure                                    |
| ------- | ------------------------------ | --------- | -------------------------------- | --------------------------------------------- |
| Source  | [source.c](src/source.c)       | File path | The whole file as one string     | Stops; nothing is scanned                     |
| Scanner | [scanner.c](src/scanner.c)     | String    | Token list ending in `EOF`       | Keeps scanning; the parser is skipped         |
| Parser  | [parser.c](src/parser.c)       | Tokens    | Sequence (metadata and steps)    | Keeps parsing; the emitter is skipped         |
| Emitter | [emitter.c](src/emitter.c)     | Sequence  | JSON text                        | Removes the partly written output file        |

* **Each stage runs only if the one before it was clean**, so the output file is never opened, and never replaced, unless the whole input is valid
* **Tokens do not copy text**; each one points back into the source string and records its length and line
* [main.c](src/main.c) reads the CLI arguments, runs the stages in order, and sets the exit code



<br>



## Tokens

The scanner reads the source one character at a time and groups characters into tokens. Whitespace and comments (`#` to the end of the line) separate tokens and are then dropped. Newlines only count lines for error messages.

| Token                                        | Form                                                               | Example                 |
| -------------------------------------------- | ------------------------------------------------------------------ | ----------------------- |
| `METADATA`, `EPOCH`, `ABSOLUTE`, `RELATIVE`  | `@` and an uppercase keyword                                       | `@EPOCH`                |
| `TIME` (absolute)                            | Exactly `YYYY-DDDTHH:MM:SS.sss`                                    | `2026-318T02:15:00.000` |
| `TIME` (relative)                            | Exactly `HH:MM:SS.sss`                                             | `00:00:02.000`          |
| `IDENTIFIER`                                 | A letter or `_`, then letters, digits, or `_`                      | `COMMAND1`, `key1`      |
| `STRING`                                     | Double-quoted, on one line, with JSON escapes                      | `"argument"`            |
| `NUMBER`                                     | JSON number: optional `-`, digits, optional fraction and exponent  | `123`, `-0.5`, `2.5E-3` |
| `TRUE`, `FALSE`                              | Lowercase only                                                     | `true`                  |
| Delimiters and separators                    | `[` `]` `{` `}` `,` `:`                                            |                         |
| `EOF`                                        | Added once, after the last character                               |                         |

* **Numbers and timestamps are scanned together**: the scanner takes the whole run of digits and letters (and `.` `:` `-` `+` when a digit follows), then checks it against the two timestamp formats and the number format
* **The scanner checks form only**; whether a timestamp is a real time, or sits after the right keyword, is the parser's job



<br>



## Grammar

```text
sequence   -> ( metadata | step )* EOF ;

metadata   -> "@METADATA" STRING value ;
step       -> time_frame TIME stem argument* ;

time_frame -> "@EPOCH" | "@ABSOLUTE" | "@RELATIVE" ;
stem       -> IDENTIFIER ;
argument   -> value ;

value      -> STRING | NUMBER | boolean | array | object ;
boolean    -> "true" | "false" ;
array      -> "[" ( value ( "," value )* )? "]" ;
object     -> "{" ( entry ( "," entry )* )? "}" ;
entry      -> IDENTIFIER ":" value ;
```

* **Statements have no terminator**: a metadata header or step ends where the next `@` keyword begins, so a step can span several lines
* **Metadata can appear anywhere**; every header is collected into the one `metadata` object
* **Metadata keys are strings; object keys are identifiers**
* **Arrays and objects** can be empty and can nest up to 256 levels deep

### Rules Outside the Grammar

| Rule                                                                 | Example of an error                  |
| -------------------------------------------------------------------- | ------------------------------------ |
| A sequence has at least one step, and the first step is `@EPOCH`     | A file with only metadata            |
| `@EPOCH` appears only once                                           | A second `@EPOCH` step               |
| `@EPOCH` and `@ABSOLUTE` take an absolute timestamp                  | `@ABSOLUTE 00:00:02.000 COMMAND1`    |
| `@RELATIVE` takes a relative timestamp                               | `@RELATIVE 2026-318T02:15:00.000 X`  |
| Day of year is `001`-`365`, or `366` in a leap year                  | `2026-366T00:00:00.000`              |
| Absolute hours are `00`-`23`; minutes and seconds are `00`-`59`      | `2026-318T24:00:00.000`              |

Relative hours are limited only by their two digits (`00`-`99`).



<br>



## Error Checks

Every error is printed to `stderr`, and both the scanner and parser continue past an error so one run reports as many as possible. An error at the end of the file reads `at end` instead of naming a token.

| Stage             | Message format                         | What is checked                                                                                                         |
| ----------------- | -------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| CLI               | `error: <message>.` and the help text  | One option per call, an input file, a path after `-o`, and an output path different from the input path                 |
| Source            | `error: <message>.`                    | The file can be opened and read                                                                                         |
| Scanner           | `error: line N: <message>`             | Unknown characters, unknown or lowercase `@` keywords, unterminated strings, invalid escapes, malformed numbers and timestamps |
| Parser (grammar)  | `error: line N at '<token>': <message>` | Missing or misplaced tokens: timestamp, stem, metadata key, value, `:`, closing `]` or `}`, trailing commas            |
| Parser (rules)    | `error: line N at '<token>': <message>` | The [rules outside the grammar](#rules-outside-the-grammar)                                                            |
| Emitter           | `error: <message>.`                    | The output file can be opened and fully written                                                                         |

### Recovery

* **Scanner**: a bad lexeme produces no token, and scanning resumes right after it
* **Parser, grammar errors**: the rest of the statement is discarded up to the next `@` keyword, and errors are silenced until then, so each statement reports one grammar error at most
* **Parser, rule errors**: the tokens are still well-formed, so parsing continues without discarding anything

### Exit Codes

| Code | Meaning                                                 |
| ---- | ------------------------------------------------------- |
| `0`  | Success, or help shown                                  |
| `64` | Usage error (bad CLI arguments)                         |
| `65` | Data error (scanner or parser errors in the input)      |
| `74` | I/O error (input not readable, or output not writable)  |



<br>



## JSON Output

| MSL                          | JSON                                                              |
| ---------------------------- | ----------------------------------------------------------------- |
| `@METADATA "key" value`      | One entry in `"metadata"`, in source order                        |
| A step                       | One object in `"steps"`, in source order                          |
| `@EPOCH`                     | `"type": "EPOCH"`                                                 |
| `@ABSOLUTE`                  | `"type": "ABSOLUTE"`                                              |
| `@RELATIVE`                  | `"type": "COMMAND_RELATIVE"`                                      |
| Timestamp                    | `"tag"`, as a string                                              |
| Stem                         | `"stem"`, as a string                                             |
| Arguments                    | `"args"` array, in source order (`[]` when a step has none)       |
| Object key (`key1`)          | Quoted string (`"key1"`)                                          |
| String, number, boolean      | Copied as written in the source                                   |

Values are not converted or reformatted; a number written as `2.5E-3` is emitted as `2.5E-3`. The one exception is a raw control character inside a string (such as a tab), which is escaped to keep the JSON valid.



<br>



## Usage and Workflow

| Call                                   | Stages run                              | Result                                        |
| -------------------------------------- | --------------------------------------- | --------------------------------------------- |
| `msl.exe <input.msl> --tokens`         | Source, scanner, parser                 | Token table on `stdout`; errors on `stderr`   |
| `msl.exe <input.msl> -o <output.json>` | Source, scanner, parser, emitter        | The output file, only if there are no errors  |
| `msl.exe --help`, or no arguments      | None                                    | Help text                                     |

1. **Write** the `.msl` file: metadata headers, an `@EPOCH` step, then `@ABSOLUTE` and `@RELATIVE` steps
2. **Inspect** with `--tokens` to see how the source was scanned (the table is printed even when the scanner finds errors)
3. **Compile** with `-o`
4. **Fix** the reported lines and compile again; an existing output file stays untouched until the input is valid

See [examples/hello_sequence.msl](examples/hello_sequence.msl) and its output, [examples/hello_sequence.msl.json](examples/hello_sequence.msl.json).
