jkparse
=======

A JSON parser for shell scripts that utilizes the (associative) array
capabilities of bash, ksh, zsh, and similar shells.  Programmed in C.
Uses the [json-c](https://github.com/json-c/json-c/wiki) library.


Why another JSON parser?
------------------------

An associative array seems like the most natural approach for transversing
a JSON object in a shell script.  Relative to alternatives, this
implementation offers strict bash and zsh compatibility, a simple interface,
strong type detection, and permits arbitrary characters in keys.
  
Bare POSIX/ash shells are not targeted by this implementation; there
are alternatives that already work well within that constraint.  This
utilizes gcc and glibc extensions which are not universally recognized.


Exampe Use
----------

	$ jkparse --help
	Typical usage: . <(jkparse [OPTIONS...] [JSON])
	  Parse JSON and return shell code for variable initialization based on the
	JSON contents.  This will read the JSON to parse either from the first non-
	option argument or, if one is not present, from stdin.  The returned shell code
	can be processed by bash v4+, ksh93, or zsh v5.5+.  Two variable declarations
	are output:
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
	  This does not stream process the input when reading from stdin; if ever input
	stream processing were implemented, this may output the variable declarations
	twice.

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
	    Include quotations around output string values, and escape as necessary to
	  generate valid JSON, so that they can be fed back through this program with
	  corresponding type detection
	 -s, --stringify
	    Take the input and output it escaped as a JSON string, without surrounding
	  quotes, whitespace, or shell escapes.  This is a formatting-only function
	  that is intended for use in constructing JSON text.  The only other option
	  that this may be logically combined with is -q, which only adds surrounding
	  quotes in the output when combined
	 -t, --type-var=JSON_TYPE
	    Specify a variable name for JSON_TYPE other than the default, JSON_TYPE.
	  If blank, the type variable will be omitted from the output
	 -u, --unset-vars
	    Output commands to unset JSON_OBJ and, if defined, JSON_OBJ_TYPES, before
	  outputting their new declarations.  This permits re-using the same variable
	  names, and using JSON_OBJ for both input and output, while transversing an
	  object
	 -v, --short-version
	    Output just the version number and exit
	 --help
	    This help screen
	 --version
	    Output version, copyright, and build options, then exit
	  Any non-empty variable name specified via an option will appear verbatim in
	the output without additional verification.  Additional options for variable
	declaration may be specified in the -a, -o, and -t option arguments.  E.g.,
	-o '-g JSON_OBJ' will promote the scope of the object's declaration in BASH.
	
	$ . <(jkparse -a OBJ_TYPES '{"number":5, "need":"input", "end":null}')
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
	$ . <(jkparse -q < /tmp/jarray)
	$ # This would also work: . <(jkparse -q "$(< /tmp/jarray)")
	$ echo $JSON_TYPE
	a
	$ echo "${JSON_OBJ[1]}" # assuming ksh indexing...
	"with
	a newline"
	
	$ echo 42 | jkparse -o '$OBJ' -t '$TYPE'
	typeset $TYPE=i;typeset $OBJ=42
	
	$ jkparse -l {23:}
	local JSON_TYPE=n;local JSON_OBJ=
	
	$ eval "$(jkparse -q '{"inventory":[{"container":{"type":"can",
	"label":"beans","weight":12.7}},{"container":{"type":"bag",
	"label":null,"weight":2.3}}],"leftHand":"flashlight"}')"
	$ . <(jkparse -qu "${JSON_OBJ[inventory]}")
	$ . <(jkparse -qu "${JSON_OBJ[1]}") // Assuming ksh indexing...
	$ . <(jkparse -ua OBJ_TYPES "${JSON_OBJ[container]}")
	$ echo ${JSON_TYPE}
	o
	$ echo ${OBJ_TYPES[weight]}
	d
	$ echo ${JSON_OBJ[type]}
	bag


Building and Installing
-----------------------

Second to the json-c library, the biggest dependency is for a printf
command that supports the %q format option.  Either a standalone printf
binary or a shell supporting this format option must be available.  The
coreutils version of printf works.  Without an override, the make target
will search the build host for a viable printf function.  If that is
okay, the following may be used for building:  

	make

For specifying a different location for a printf binary:  

	make PRINTF_EXECUTABLE=/bin/printf

If using a shell's built-in printf function:  

	make USE_SHELL_PRINTF=/bin/ksh

Once compiled, install the output executable in a suitable location as
root.  The default install target places it in /usr/local/bin:  

	sudo make install


Alternatives
------------

JSON parsers for shell use (some of these, like jq, generate JSON as well):  
[jsoncvt](https://github.com/krz8/jsoncvt) can also directly output a
ksh associative array.  It has less dependencies, human readable output,
and supports generating multidimensional arrays.  
[jq](https://stedolan.github.io/jq/), a de-facto standard.  
[jshon](http://kmkeen.com/jshon/)  
[JSON.sh](https://github.com/dominictarr/JSON.sh)  
[json.sh](https://github.com/rcrowley/json.sh)  
[TickTick](https://github.com/kristopolous/TickTick)  
[libshell](https://github.com/legionus/libshell) has a lot of capabilities,
including JSON parsing.  
[posix-awk-shell-jq](https://github.com/vcheckzen/posix-awk-shell-jq)  
[json-to-sh](https://github.com/mlvzk/json-to-sh)  
[jwalk](https://github.com/shellbound/jwalk/)  
[jsawk](https://github.com/micha/jsawk)  
  
JSON generators for shell use:  
[jo](https://github.com/jpmens/jo)  
[json_pp](https://github.com/deftek/json_pp)
