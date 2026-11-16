# Mission Sequencing Domain-Specific Language (DSL)

A domain-specific language for spaceflight mission sequencing.



<br>



## Table of Contents

1. [Rationale](#rationale)
2. [References and Sources](#references-and-sources)



<br>



## Rationale

Mission sequencing in spaceflight autonomy must not pose risks. Actions should be deterministic and highly controlled in order to ensure success, as failure in spaceflight is often nothing short of expensive. Consequently, this project aims to design a domain-specific language for mission sequencing written in C. The language aims to only express what a sequence calls for, stripping away any volatile, risk-posing architecture.



<br>



## References and Sources

* [seq-json-schema](https://github.com/NASA-AMMOS/seq-json-schema)
* [DevDocs - C documentation](https://devdocs.io/c/)
* [Crafting Interpreters](https://craftinginterpreters.com/contents.html)
    * [Scanning](https://craftinginterpreters.com/scanning.html)
    * [Parsing](https://craftinginterpreters.com/parsing-expressions.html)
* [Managing releases in a repository](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)
