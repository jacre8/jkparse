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
- Added a note in the online help about how BASH's -g declaration option may be included in the output


4 - 2022-05-01
==============

After a few quick version interations, this may be considered to be the initial release
