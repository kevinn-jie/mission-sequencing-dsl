# Mission Sequencing Language (MSL) Design Choices



## Design Pillars

The name for this DSL is Mission Sequencing Language (MSL). MSL should be simplistic, human readable, and more efficient to type than JSON.

### General Step Structure

```text
[time frame] [stem] [arguments]
```

### General Output JSON Structure

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

### File Extensions

* `.msl`: Source code file
* `.msl.json`: Output file



<br>



## Keywords

* `#`: Comments
* `@METADATA`: Metadata headers
* `@EPOCH`: Start of sequence
* `@ABSOLUTE`: Absolute timestamp
* `@RELATIVE`: Relative timestamp

### Keyword Rules

* Keywords are case-sensitive: only the uppercase forms above are valid (`@epoch` is an error)
* An `@` followed by any other word (`@FOO`) or by no word at all is an error
* Comments run from `#` to the end of the line



<br>



## Delimeters

* `[]`
* `{}`



<br>



## Separators

* `,`: Separates array elements and object entries
* `:`: Separates an object key from its value



<br>



## Whitespace

* Spaces, tabs, carriage returns, and newlines separate tokens but are not tokens themselves
* Newlines are not significant; each step begins at its time frame keyword (`@EPOCH`, `@ABSOLUTE`, `@RELATIVE`)



<br>



## Literals

* Array
* Identifier
* String
* Boolean
* Number
* Timestamp

### Array

* Builds `[`, `,`, and `]` by the parser; the scanner has no array token

### Identifier

* Starts with a letter or `_`, followed by letters, digits, or `_`
* Used for stems (`COMMAND1`) and object keys (`key1`)

### String

* Enclosed in double quotes: `"argument"`
* Must fit on one line; a line break before the closing quote is an error
* Supports JSON escapes: `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`, and `\uXXXX` (exactly 4 hex digits)
* Any other escape is an error

### Boolean

* Lowercase only: `true` and `false`
* `TRUE` and `True` are identifiers, not booleans

### Number

* **Integers and decimals**: `123`, `0.123`
* **Negative**: `-5`, `-0.5`
* **Exponent**: `1e5`, `2.5E-3`
* A decimal point needs digits on both sides: `.5` and `5.` are errors
* A leading `+` is an error: `+5`

### Timestamp

* **Absolute**: exactly `YYYY-DDDTHH:MM:SS.sss`, where `DDD` is the day of year (`2026-318T02:15:00.000`)
* **Relative**: exactly `HH:MM:SS.sss` (`00:00:02.000`)
* Every field has a fixed number of digits; `2026-318T2:15:00` and `0:2.5` are errors
* Timestamps cannot be negative



<br>



## Lexical Errors

* Each error is reported with its line number
* Scanning continues after an error so every error in a file is reported in one run
