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


## Keywords

* `#`: Comments
* `@METADATA`: Metadata headers
* `@EPOCH`: Start of sequence
* `@ABSOLUTE`: Absolute timestamp
* `@RELATIVE`: Relative timestamp



## Delimeters

* `()`
* `[]`
* `{}`



## Literals

* Array
* Identifier
* String
* Boolean
* Number