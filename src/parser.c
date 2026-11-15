/**
 * @file  parser.c
 * @brief Parses a list of tokens into a sequence.
 */

/*
 * MSL grammar
 *
 * Rules:
 *     A sequence is every metadata header, then every step, then the end of the file.
 *         sequence   -> metadata* step* EOF ;
 *
 *     A metadata header is a key and its value, both strings.
 *         metadata   -> "@METADATA" STRING STRING ;
 *         e.g.          @METADATA "author" "kevin_jie"
 *
 *     A step is a time frame, a timestamp, a stem, and any number of arguments.
 *     A step ends where the next time frame keyword begins.
 *         step       -> time_frame TIME stem argument* ;
 *         time_frame -> "@EPOCH" | "@ABSOLUTE" | "@RELATIVE" ;
 *         stem       -> IDENTIFIER ;
 *         argument   -> value ;
 *         e.g.          @RELATIVE 00:00:20.000 COMMAND3 "argument" [123, true]
 *
 *     A value is a string, a number, a boolean, an array, or an object.
 *         value      -> STRING | NUMBER | boolean | array | object ;
 *         boolean    -> "true" | "false" ;
 *
 *     An array is a comma-separated list of values inside brackets.
 *         array      -> "[" elements? "]" ;
 *         elements   -> value ( "," value )* ;
 *         e.g.          ["argument", 123, true]
 *
 *     An object is a comma-separated list of entries inside braces.
 *     An entry is an identifier key, a colon, and a value.
 *         object     -> "{" entries? "}" ;
 *         entries    -> entry ( "," entry )* ;
 *         entry      -> IDENTIFIER ":" value ;
 *         e.g.          {key1: "argument", key2: 123}
 *
 * Checks made outside the grammar:
 *     The scanner emits absolute and relative timestamps as the same TIME token
 *     and checks their format only, so the parser checks that:
 *       - TIME is absolute (YYYY-DDDTHH:MM:SS.sss) after "@EPOCH" and
 *         "@ABSOLUTE".
 *       - TIME is relative (HH:MM:SS.sss) after "@RELATIVE".
 *       - Each TIME field is in range (day of year, hour, minute, second).
 *
 * Unconfirmed choices in the rules above:
 *     Ordering:
 *       - A metadata header after the first step is an error.
 *       - "@EPOCH" is optional, may repeat, and may appear at any step.
 *     Values:
 *       - A metadata value must be a string.
 *       - Arrays and objects may be empty: [] and {}
 *       - Arrays and objects may nest: [[1, 2], {key1: [3]}]
 *       - A trailing comma is an error: [1, 2,]
 *     Unused tokens:
 *       - "(" and ")" are scanned, but no rule uses them, so they are always an error.
 * 
 * Choice confirmation:
 *     Ordering:
 *       - Metadata headers can appear anywhere, they simply translate to the metadata section.
 *       - "@EPOCH" is required because it denotes the beginning of a sequence.
 *     Values:
 *       - Metadata values can be booleans, strings, numbers, arrays, and objects, but the key must be a string.
 *         Identifiers do no work with metadata.
 *       - Arrays and objects can be empty: [] and {}
 *       - Arrays and objects can nest: [[1, 2], {key1: [3]}]
 *       - A trailing comma is an error: [1, 2,]
 *     Unused tokens:
 *       - They are not needed, they can be removes and unintegrated.
 */

