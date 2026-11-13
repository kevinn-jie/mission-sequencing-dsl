# Mission Sequencing Language (MSL)



## Design Pillars

MSL should be simplistic, human readable, and more efficient to type than JSON.

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



<br>



## Delimeters

* `()`
* `[]`
* `{}`



<br>



## Literals

* Array
* Identifier
* String
* Boolean
* Number