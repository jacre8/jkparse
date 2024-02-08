10 - 2024-02-07
==============

- Bugfix: the --quote-strings option is now honored when combined with the --stringify option even
when an empty string is input over stdin
- Added an error message for a read or memory allocation failure when reading from stdin when
the --stringify and --verbose options are both specified


9 - 2024-02-07
==============

- Bugfix: reading in an empty string over stdin with the --stringify option would result in a
segmentation fault
- jkparseOutputToJson.sh bugfixes: encoding of strings beginning with a dash character ('-') would
fail
- Added jkparseOutputToJson.sh to the Makefile's install target.  It is placed in the same bin
directory as jkparse and marked executable so that it can be sourced with tab completion
- Re-added '.PHONY : install' to the Makefile, after accidently removing


8 - 2023-07-08
==============

- Specifying an empty value with the -e/--empty-key option will now result in object values with
empty keys being excluded from the output
- jkparseObjToJson(), in jkparseOutputToJson.sh, now considers the placeholder value for empty
keys in objects and will generate JSON objects containing empty keys when that key value matches
- jkparseOutputToJson() and jkparseObjToJson() now accept an additional argument for specifying
the object empty key placeholder value
- Added clean target to Makefile


7 - 2023-06-18
==============

- Bugfix: newline characters are no longer escaped in associative array key values
- Added parse error reporting, both in the program's return code and more descriptively via the
new --verbose option
- The --quote-strings option will now also result in null object values being output as the string,
null, rather than as empty strings
- The jkparseArrayToJson() and jkparseObjToJson() functions in jkparseOutputToJson.sh now properly
format null values
- The jkparseObjToJson() shell function now properly encodes key values as JSON strings


6 - 2023-06-16
==============

- When parsing with the -q / --quote-strings option specified, the type indicator for strings will
will now be 'q' instead of 's'.  The intent with this change is to facilitate the implementation of
generic formatting functions that can operate on the output regardless of whether or not this
option was specified.  This behavior of -q conflicts with previous versions, although the -q option
was broken in older versions (before yesteday) anyway.


5 - 2023-06-15
==============

- Bugfix: appropriate JSON escape sequences are now included in strings output with the
-q / --quote-strings option
- Added the -s / --stringify option to aide in generating JSON
- Added a note in the online help about how BASH's -g declaration option may be included in the
output


4 - 2022-05-01
==============

After a few quick version interations, this may be considered to be the initial release
