2024-02-12
==========

- libjkparse.sh v2
	- Added workarounds for variable scoping issues in ksh, with possible namespace caveats as
noted in the comments.  jkparseSet() now works in ksh
	- jkparseArrayToJson() now fills in the missing indices of sparse arrays in bash and ksh so
that the output has the same indices as the array did in the shell.  A new function,
jkparseCompactArrayToJson(), contains the old behavior of dropping missing indices in the output,
although it is faster and suitable in cases where dropped indices are not a concern
	- Implemented Q versions of the jkparseToJson functions that are a little faster but depend on
the JSON having been parsed using jkparse's -q option
	- Optimized jkparseSet() implementation


2024-02-11
==========

- jkparse 11
	- The generated code now has a non-zero exit code when there is a parse error
	- Bugfix: return non-zero upon a parse error when --obj-var is specified as empty
- Renamed jkparseOutputToJson.sh to libjkparse.sh
- libjkparse.sh v1
	- Added jkparseGet() and jkparseSet()
	- Renamed jkparseOutputToJson() to jkparseToJson()
	- Exports LIBJKPARSE_VERSION
	- Bugfix: zsh output functions would erroneously interpret escapes inside strings


2024-02-10
==========

- jkparseOutputToJson.sh
	- Function definitions now include zsh compatible versions
	- Bugfix: jkparseOutputToJson() can now encode an unquoted string that begins with a dash
character ('-')


2024-02-07
==========

- jkparse 10
	- Bugfix: the --quote-strings option is now honored when combined with the --stringify option
even when an empty string is input over stdin
	- Added an error message for a read or memory allocation failure when reading from stdin when
the --stringify and --verbose options are both specified
- jkparse 9
	- Bugfix: reading in an empty string over stdin with the --stringify option would result in a
segmentation fault
- jkparseOutputToJson.sh bugfixes: encoding of objects and arrays containing strings beginning with
a dash character ('-') would fail
- Makefile
	- Re-added '.PHONY : install' to the Makefile, after accidently removing
	- Added jkparseOutputToJson.sh to the Makefile's install target.  It is placed in the same bin
directory as jkparse and marked executable so that it can be sourced with tab completion


2023-07-08
==========

- jkparse 8
	- Specifying an empty value with the -e/--empty-key option will now result in object values
with empty keys being excluded from the output
- jkparseOutputToJson.sh
	- jkparseObjToJson(), in jkparseOutputToJson.sh, now considers the placeholder value for empty
keys in objects and will generate JSON objects containing empty keys when that key value matches
	- jkparseOutputToJson() and jkparseObjToJson() now accept an additional argument for specifying
the object empty key placeholder value
- Added clean target to Makefile


2023-06-18
==========

- jkparse 7
	- Bugfix: newline characters are no longer escaped in associative array key values
	- Added parse error reporting, both in the program's return code and more descriptively via the
new --verbose option
	- The --quote-strings option will now also result in null object values being output as the string,
null, rather than as empty strings
- jkparseOutputToJson.sh
	- The jkparseArrayToJson() and jkparseObjToJson() functions in jkparseOutputToJson.sh now
properly format null values
	- The jkparseObjToJson() shell function now properly encodes key values as JSON strings


2023-06-17
==========

- Added jkparseOutputToJson.sh to the repository


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
