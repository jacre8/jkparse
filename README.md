jkparse
=======

A JSON parser for shell scripts that utilizes the (associative) array
capabilities of bash, ksh, zsh, and similar shells.  Programmed in C.
Uses the [json-c](https://github.com/json-c/json-c/wiki) library.

Why another JSON parser?
------------------------

An associative array seems like the most natural approach for
transversing a JSON object in a shell script.  I could not find a parser
intended for shell use that easily exports a JSON object to a shell
array, so I set about creating one.  I did not look very hard...  A
pre-existing alternative that I later found is [jsoncvt](https://github.com/krz8/jsoncvt).
I haven't tried that one out yet.  Relative to that, this one may be
preferable if you already have a json-c library dependency elsewhere.  
  
This implementation prioritizes simplicity and efficiency over
portability.  

Exampe Use
----------

	$ jkparse --help
	Typical usage: . <(jkparse [OPTIONS...] [JSON])
	  Parse JSON and return shell code for variable initialization based on the
	JSON contents.  The returned shell code can be processed by bash v4+, ksh93,
	or zsh v5.5+.  Two variable declarations are output:
	  JSON_TYPE - this is a single character describing the detected type of the
	JSON argument.  This character is the first character for one of the following
	types: null, boolean, int, double, string, array, or object.  The type will be
	null if JSON cannot be represented by one of the other types.
	  JSON_OBJ - this is the parsed result.  It is formatted based on JSON_TYPE as
	one of the following:
	 null - empty string
	 boolean - string containing either 'true' or 'false'
	 int, double - decimal string value
	 string - string value without quotes
	 array - array containing a string representation of each member
	 object - associative array containing a string representation of each member
	  When JSON is either an array or an object type, non-string typed members of
	the output array can be fed back through this program for further processing.
	String typed members can also be fed back through if the --quote-strings option
	is specified when the array is generated.
	  There is no special handling for duplicated keys in objects.  When there are
	duplicate keys, multiple assignments will be output in the order that the keys
	appear in the original JSON.

	OPTIONS:
	 -a, --array-var=JSON_OBJ_TYPES
	    When JSON_OBJ is either an array or object type, declare a third variable,
	  named JSON_OBJ_TYPES, that contains an array or associative array containing
	  characters corresponding to the types of the array or object's members,
	  respectively.  The characters in this array are the same characters used in
	  JSON_TYPE.  When JSON_OBJ_TYPES is an empty string, which is the default,
	  this variable declaration is omitted from the output
	 -e, --empty-key=EMPTY_KEY
	    Empty keys are valid in JSON but not in shell script arrays.  Specify a
	  string to replace empty keys with.  The default is "$'\1'".  This value
	  must be suitable for shell use.  No verification or substitution in output is
	  made for a non-empty value that is specified here.  An empty value is invalid
	 -l, --local-declarations
	    Declare variables using the local keyword rather than the default, typeset
	 -o, --obj-var=JSON_OBJ
	    Specify a variable name for JSON_OBJ other than the default, JSON_OBJ.
	  If blank, the object and array variables will be omitted from the output
	 -q, --quote-strings
	    Include quotations around output string values so that they can be fed back
	  through this program with corresponding type detection
	 -t, --type-var=JSON_TYPE
	    Specify a variable name for JSON_TYPE other than the default, JSON_TYPE.
	  If blank, the type variable will be omitted from the output
	 -v, --short-version
	    Output just the version number and exit
	 --help
	    This help screen
	 --version
	    Output version, copyright, and build options, then exit
	  Any non-empty variable name specified via an option will appear verbatim in
	the output without additional verification.

	$ . <(jkparse -a OBJ_TYPES '{"number":5, "need":"input", "end":null}'
	$ echo $JSON_TYPE
	o
	$ echo ${JSON_OBJ[number]}
	5
	$ echo ${OBJ_TYPES[need]}
	s
	$ echo ${JSON_OBJ[need]}
	input
	$ echo ${OBJ_TYPES[end]}
	n

	$ printf '["string", "with\na newline", 5]' > /tmp/jarray
	$ . <(jkparse -q "$(< /tmp/jarray)")
	$ echo $JSON_TYPE
	a
	$ echo "${JSON_OBJ[1]}" # assuming bash/ksh here for the index value
	"with
	a newline"
	
	$ jkparse 42
	typeset JSON_TYPE=i
	typeset JSON_OBJ=42
	
	$ jkparse -l {23:}
	local JSON_TYPE=n
	local JSON_OBJ=


Building and Installing
-----------------------
Second to the json-c library, the biggest dependency is for a printf
command that supports the %q format option.  Either a standalone printf
binary or a shell supporting this format option must be available.  The
coreutils version of printf works.  If that is okay, and it is in the
standard location of /usr/bin/printf, the following may be used for
building:  

	gcc -O2 -o jkparse jkparse.c -ljson-c
For specifying a different location for a printf binary:  

	gcc -D 'PRINTF_EXECUTABLE="/bin/printf"'-O2 -o jkparse jkparse.c -ljson-c
If using a shell's built-in printf function:  

	gcc -D 'USE_SHELL_PRINTF="/bin/ksh"' -O2 -o jkparse jkparse.c -ljson-c
Once compiled, as root, install the output executable in a location
accesible via $PATH, e.g.:  

	sudo install -o root -g root -m 0755 jkparse /usr/local/bin

Alternatives
------------

[jsoncvt](https://github.com/krz8/jsoncvt)  
[jq](https://stedolan.github.io/jq/)  
[jshon](http://kmkeen.com/jshon/)  
[JSON.sh](https://github.com/dominictarr/JSON.sh)  
[json.sh](https://github.com/rcrowley/json.sh)  
[TickTick](https://github.com/kristopolous/TickTick)  
[libshell](https://github.com/legionus/libshell)  
[posix-awk-shell-jq](https://github.com/vcheckzen/posix-awk-shell-jq)  
[json-to-sh](https://github.com/mlvzk/json-to-sh)
