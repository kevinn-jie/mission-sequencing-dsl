# Mission Sequencing Domain-Specific Language (DSL)



## Table of Contents

1. [Rationale](#rationale)



<br>



## Rationale

Mission sequencing in spaceflight autonomy must not pose risks. Actions should be deterministic and highly controlled in order to ensure success, as failure in spaceflight is often nothing short of expensive. Consequently, this project aims to design a domain-specific language for mission sequencing written in C. The language aims to only express what a sequence calls for, stripping away any volatile, risk-posing architecture.