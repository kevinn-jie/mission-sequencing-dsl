# Mission Sequencing Domain-Specific Language (DSL)



## Table of Contents

1. [Rationale]()



<br>



## Rationale

Mission sequencing in spaceflight systems are frequently written in general-purpose scripting or mission-specific tooling languages, where behavior depends on the compiler. However, behavior can differ between compilers, volatility in non-deterministic in some languages, and sequencing errors cannot be risked.

Consequently, this project aims to design a domain-specific language for mission sequencing written in C. The language will only express what a sequence needs: timing, commands, conditions, constraints, and strict customization.