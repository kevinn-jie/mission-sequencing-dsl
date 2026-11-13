# Mission Sequencing Language (MSL)



## Design Pillars

* MSL should be simplistic, human readable, and more efficient to type than JSON.
* To be compatible per team, recognized commands and identifers should be easily interchangeable.



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
* **Types**: Defined and function per MSL implementation, act as keywords
* **Strings**: Denoted using `""`
* **Numbers**: Contiuous digits, including periods