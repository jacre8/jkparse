# libjkparse.sh
#  Helper shell functions for jkparse, including functions for converting
# output from jkparse, presumably after modifications, back to JSON.  This is
# intended to be sourced, although there are no license restrictions to prevent
# copying the definitions within this file into another work.
export LIBJKPARSE_VERSION=1 # previously named jkparseOutputToJson.sh

#  Copyright 2023-2024 Jason Hinsch
#  No restrictions on use; Public Domain.

#  The function definitions herein are intended to be compatible with bash,
# ksh, and zsh.  The output of these functions is fidelious and functional,
# but not pretty.  Their output also does not include a trailing newline.
# Consider piping their output through another tool for more human readable
# output, such as jq or jsoncat.

#  The following functions are defined here:

# jkparseGet()
#  $@ = key and/or index values of nested object to retrieve
#  Take a JSON object or array over stdin and output a nested JSON object over
# stdout that is suitable for input into jkparse.  E.g. if the embedded object
# is a string, it will be quoted in the output.  Arguments specify successive
# key/index fields, starting from the top level object.  Array indices follow
# the convention of the shell, rather than JSON's convention.  Returns non-zero
# without output if there is an error in parsing, there is no matching object,
# or if there are no arguments.

# jkparseSet()
#  ${@:1:$((${#@} - 1))} = key and/or index values of nested object to modify
#  ${@:${#@}} = new JSON value for the specified nested field
#  Take a JSON object or array over stdin, modify or add a nested field inside
# that object using a JSON encoded value specified in the last argument, and
# output the new object over stdout.  Arguments before the last specify
# successive key/index fields, starting from the top level object.  Array
# indices follow the convention of the shell, rather than JSON's convention.
# Returns non-zero without output if there is an error in parsing, there is a
# type mismatch in the assignment, the new value is not valid JSON, or if there
# are insufficient arguments.

# jkparseToJson()
#  $1 = $JSON_TYPE
#  $2 = name of JSON_OBJ
#  $3 = name of JSON_OBJ_TYPES
#  $4 (optional) = object EMPTY_KEY string (the default, if undefined, is $'\1')
#  Take variables created using jkparse and output JSON.  $3 can be the name of
# an empty associative array, in which case it will be assumed that the jkparse
# output was generated using the -q option when $1 = a or o.  If the optional
# argument $4 is defined, object keys that match it will be treated as empty
# strings in the output.  If $4 is undefined, keys matching the string $'\1'
# will be treated as empty key strings.

# jkparseObjToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  $3 (optional) = object EMPTY_KEY string (the default, if undefined, is $'\1')
#  Take the associative array variable whose name is in $1, with types in
# the associative array variable whose name is in $2, and print it as JSON.
# $2 can be the name of an empty associative array, in which case it will be
# assumed that the jkparse output was generated using the -q option.  If the
# optional argument $3 is defined, keys that match it will be treated as
# empty strings in the output.  If $3 is undefined, keys matching the string
# $'\1' will be treated as empty key strings.

# jkpaseArrayToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  Take the array variable whose name is in $1, with types in the array
# variable whose name is in $2, and print it as JSON.  $2 can be the name of an
# empty array, in which case it will be assumed that the jkparse output was
# generated using the -q option.

# jkparseContinuousArrayToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  Like jkparseArrayToJson(), and possibly faster, but it does not handle
# missing elements in bash and ksh gracefully.

function jkparseGet
{
	[ ${#@} -gt 0 ] &&
	. <(jkparse -q) && {
		[ $JSON_TYPE = o ] || [ $JSON_TYPE = a ]
	} && {
		while [ ${#@} -gt 1 ];do
			. <(jkparse -qu <<< "${JSON_OBJ[$1]}") && {
				[ $JSON_TYPE = o ] || [ $JSON_TYPE = a ]
			} || return
			shift 1
		done
		[ "${JSON_OBJ[$1]}" ] && printf %s "${JSON_OBJ[$1]}"
	}
}

function jkparseSet
{
	. <(jkparse -qa OBJ_TYPES) && {
		[ $JSON_TYPE = o ] || [ $JSON_TYPE = a ]
	} &&
	if [ ${#@} -lt 3 ];then
		. <(jkparse -qo '' -t childType <<< "$2") &&
		JSON_OBJ[$1]=$2 &&
		OBJ_TYPES[$1]=$childType
	else
		JSON_OBJ[$1]=$(jkparseSet "${@:2}" <<< "${JSON_OBJ[$1]}")
	fi &&
	if [ $JSON_TYPE = o ];then
		jkparseObjToJson JSON_OBJ OBJ_TYPES
	else
		jkparseArrayToJson JSON_OBJ OBJ_TYPES
	fi
}

jkparseToJson ()
{
	case $1 in
		b|d|i|q) eval printf %s \"\$"$2"\";;
		a) jkparseArrayToJson "$2" "$3";;
		o) eval jkparseObjToJson '$2' '$3' \"\${4-$'\1'}\";;
		s) eval jkparse -qs -- \"\$"$2"\";;
		n) echo -n null;;
	esac
}

#  To avoid any possible name conflicts, the names of local variables inside
# these functions incorporate all variable argument names.
if [ -n "$ZSH_VERSION" ];then
	jkparseObjToJson ()
	{
		#  key and comma flag local variables
		echo -n '{'
		eval "local k$1$2 c$2$1=
		for k$1$2 in \"\${(@k)$1}\";do
			[ -z \"\$c$2$1\" ] || echo -n ,
			c$2$1=1
			if [ \"\$k$1$2\" = \"\${3-$'\1'}\" ];then
				printf '\"\"'
			else
				jkparse -qs -- \"\$k$1$2\"
			fi
			case \"\${$2[\$k$1$2]}\" in
				s) echo -n :
				   jkparse -qs -- \"\${$1[\$k$1$2]}\";;
				n) echo -n :null;;
				*) printf %s :\"\${$1[\$k$1$2]}\";;
			esac
		done"
		echo -n '}'
	}
	
	#  zsh does not create sparse arrays
	jkparseArrayToJson ()
	{
		#  index local variable
		echo -n [
		eval "local i$1$2=0
		while [ \$i$1$2 -lt \${#$1[@]} ];do
			[ \"\$((i$1$2++))\" = 0 ] || echo -n ,
			case \"\${$2[\$i$1$2]}\" in
				s) jkparse -qs -- \"\${$1[\$i$1$2]}\";;
				n) echo -n null;;
				*) printf %s \"\${$1[\$i$1$2]}\";;
			esac
		done"
		echo -n ]
	}
	alias jkparseContinuousArrayToJson=jkparseArrayToJson
else
	function jkparseObjToJson
	{
		#  key and comma flag local variables
		echo -n '{'
		eval "typeset k$1$2 c$2$1=
		for k$1$2 in \"\${!$1[@]}\";do
			[ -z \"\$c$2$1\" ] || echo -n ,
			c$2$1=1
			if [ \"\$k$1$2\" = \"\${3-$'\1'}\" ];then
				printf '\"\"'
			else
				jkparse -qs -- \"\$k$1$2\"
			fi
			case \"\${$2[\$k$1$2]}\" in
				s) echo -n :
				   jkparse -qs -- \"\${$1[\$k$1$2]}\";;
				n) echo -n :null;;
				*) echo -n :\"\${$1[\$k$1$2]}\";;
			esac
		done"
		echo -n '}'
	}
	
	#  This version handles sparse arrays (arrays that are missing indices).
	function jkparseArrayToJson
	{
		#  key and comma flag local variables
		echo -n [
		eval "typeset k$1$2 c$2$1=
		for k$1$2 in \"\${!$1[@]}\";do
			[ -z \"\$c$2$1\" ] || echo -n ,
			c$2$1=1
			case \"\${$2[\$k$1$2]}\" in
				s) jkparse -qs -- \"\${$1[\$k$1$2]}\";;
				n) echo -n null;;
				*) echo -n \"\${$1[\$k$1$2]}\";;
			esac
		done"
		echo -n ]
	}

	#  This may be faster than jkparseArrayToJson(), but it does not handle
	# missing elements gracefully.
	function jkparseContinuousArrayToJson
	{
		#  index local variable
		echo -n [
		eval "typeset i$1$2=-1
		while [ \$((++i$1$2)) -lt \${#$1[@]} ];do
			[ \"\$i$1$2\" = 0 ] || echo -n ,
			case \"\${$2[\$i$1$2]}\" in
				s) jkparse -qs -- \"\${$1[\$i$1$2]}\";;
				n) echo -n null;;
				*) echo -n \"\${$1[\$i$1$2]}\";;
			esac
		done"
		echo -n ]
	}
fi
