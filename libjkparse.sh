# libjkparse.sh
#  Helper shell functions for jkparse, including functions for converting
# output from jkparse, presumably after modifications, back to JSON.  This is
# intended to be sourced, although there are no license restrictions to prevent
# copying the definitions within this file into another work.
export LIBJKPARSE_VERSION=2

#  Copyright 2023-2024 Jason Hinsch
#  No restrictions on use; Public Domain.

#  The function definitions herein are intended to be compatible with bash,
# ksh, and zsh.  The output of these functions is fidelious and functional,
# but not pretty.  Their output also does not include a trailing newline.
# Consider piping their output through another tool for more human readable
# output, such as jq or jsoncat.

#  There are potential namespace conflicts in ksh with the jkparseToJson
# functions.  Ideally, the jkparseToJson functions would have both their
# own local variables and have access to their caller's variables.  In ksh,
# however, it is not possible for a function to have both -- functions declared
# with the function keyword cannot access variables declared by a calling
# function that, itself, was also declared with the function keyword.  The
# current implementation of the jkparseToJson functions will declare their
# variables in the caller's namespace rather than function local, which 
# permits them access to the named variables at the cost of potential name
# conflicts.  ksh93u+m has plans to implement the local keyword which, if used
# here, could resolve this issue.
#  This is not a concern in bash which, using the same syntax, declare's its
# variables function local while still having access to the callers variables.

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

# jkparseArrayToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  Take the array variable whose name is in $1, with types in the array
# variable whose name is in $2, and print it as JSON.  $2 can be the name of an
# empty array, in which case it will be assumed that the jkparse output was
# generated using the -q option.

# jkparseCompactArrayToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  bash and ksh support sparse arrays: arrays with non-continuous indices.
# The regular jkpaseArrayToJson function will interpret gaps in the indices as
# members containing null values, so that the indices in the generated JSON
# will correspond to the indices of the source shell's array.  In some use
# cases this may be undesirable, or it may be already assured that the shell
# array does not contain gaps, such as when jkparse generated the source array.
# jkparseCompactArrayToJson() exists for these use cases.  This function will
# skip missing indices in the resulting JSON if the source array is sparse.  It
# is slightly faster than jkpaseArrayToJson(), although it does not guarantee
# that the indices align with the source in all cases.

# jkparseContinuousArrayToJson()
#  $1 = name of JSON_OBJ
#  $2 = name of JSON_OBJ_TYPES
#  Possibly faster than jkparseCompactArrayToJson(), but does not handle
# missing elements in bash and ksh gracefully.

# jkparseQToJson()
#  $1 = $JSON_TYPE
#  $2 = name of JSON_OBJ
#  $3 (optional) = object EMPTY_KEY string (the default, if undefined, is $'\1')
# jkparseQObjToJson()
#  $1 = name of JSON_OBJ
#  $2 (optional) = object EMPTY_KEY string (the default, if undefined, is $'\1')
# jkparseQArrayToJson()
#  $1 = name of JSON_OBJ
# jkparseQCompactArrayToJson()
#  $1 = name of JSON_OBJ
# jkparseQContinuousArrayToJson()
#  $1 = name of JSON_OBJ
#  These are similar to functions of the same name without the Q, except that
# they do not accept a JSON_OBJ_TYPES name argument.  These function versions
# expect that the source variables were setup using jkparse's -q option.  These
# are slightly faster than the versions that have additional type handling.


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
	. <(jkparse -q) && {
		[ $JSON_TYPE = o ] || [ $JSON_TYPE = a ]
	} &&
	if [ ${#@} -lt 3 ];then
		. <(jkparse -qo '' -t '' <<< "$2") &&
		JSON_OBJ[$1]=$2
	else
		JSON_OBJ[$1]=$(jkparseSet "${@:2}" <<< "${JSON_OBJ[$1]}")
	fi &&
	if [ $JSON_TYPE = o ];then
		jkparseQObjToJson JSON_OBJ
	else
		jkparseQContinuousArrayToJson JSON_OBJ
	fi
}

jkparseToJson ()
{
	case $1 in
		b|d|i|q) eval printf %s \"\$"$2"\";;
		o) eval jkparseObjToJson '$2' '$3' \"\${4-$'\1'}\";;
		a) jkparseArrayToJson "$2" "$3";;
		s) eval jkparse -qs -- \"\$"$2"\";;
		n) echo -n null;;
	esac
}

jkparseQToJson ()
{
	case $1 in
		o) eval jkparseQObjToJson '$2' \"\${3-$'\1'}\";;
		a) jkparseQArrayToJson "$2" ;;
		*) eval printf %s \"\$"$2"\";;
	esac
}

#  To avoid any possible name conflicts with the name arguments, the names of
# local variables inside these functions incorporate all variable argument
# names.
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
	
	jkparseQObjToJson ()
	{
		#  key and comma flag local variables
		echo -n '{'
		eval "local k$1 c$1=
		for k$1 in \"\${(@k)$1}\";do
			[ -z \"\$c$1\" ] || echo -n ,
			c$1=1
			if [ \"\$k$1\" = \"\${2-$'\1'}\" ];then
				printf '\"\"'
			else
				jkparse -qs -- \"\$k$1\"
			fi
			printf %s :\"\${$1[\$k$1]}\"
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
	
	jkparseQArrayToJson ()
	{
		#  index local variable
		echo -n [
		eval "local i$1=0
		while [ \$i$1 -lt \${#$1[@]} ];do
			[ \"\$((i$1++))\" = 0 ] || echo -n ,
			printf %s \"\${$1[\$i$1]}\"
		done"
		echo -n ]
	}
	
	alias jkparseCompactArrayToJson=jkparseArrayToJson
	alias jkparseQCompactArrayToJson=jkparseQArrayToJson
	alias jkparseContinuousArrayToJson=jkparseArrayToJson
	alias jkparseQContinuousArrayToJson=jkparseQArrayToJson
else
	jkparseObjToJson ()
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
	
	jkparseQObjToJson ()
	{
		#  key and comma flag local variables
		echo -n '{'
		eval "typeset k$1 c$1=
		for k$1 in \"\${!$1[@]}\";do
			[ -z \"\$c$1\" ] || echo -n ,
			c$1=1
			if [ \"\$k$1\" = \"\${2-$'\1'}\" ];then
				printf '\"\"'
			else
				jkparse -qs -- \"\$k$1\"
			fi
			echo -n :\"\${$1[\$k$1]}\"
		done"
		echo -n '}'
	}
	
	#  When the shell array is sparse, this version will generate null values
	# for the missing indices in the resulting JSON.
	jkparseArrayToJson ()
	{
		#  index and limit local variables.
		echo -n [
		eval "typeset i$1$2=-1 l$2$1
		for l$2$1 in \"\${!$1[@]}\";do true; done
		let l$2$1++
		while [ \$((++i$1$2)) -lt \$l$2$1 ];do
			[ \"\$i$1$2\" = 0 ] || echo -n ,
			case \"\${$2[\$i$1$2]}\" in
				s) jkparse -qs -- \"\${$1[\$i$1$2]}\";;
				n) echo -n null;;
				*) echo -n \"\${$1[\$i$1$2]-null}\";;
			esac
		done"
		echo -n ]
	}
	
	jkparseQArrayToJson ()
	{
		#  index and limit local variables.
		echo -n [
		eval "typeset i$1=-1 l$1
		for l$1 in \"\${!$1[@]}\";do :; done
		let l$1++
		while [ \$((++i$1)) -lt \$l$1 ];do
			[ \"\$i$1\" = 0 ] || echo -n ,
			echo -n \"\${$1[\$i$1]-null}\"
		done"
		echo -n ]
	}
	
	#  When the shell array is sparse, this version will skip missing indices
	# in the resulting JSON.  A sparse array will not have the same indices in
	# JSON as it did in the shell (zsh's count from 1 notwithstanding)
	jkparseCompactArrayToJson ()
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
	
	jkparseQCompactArrayToJson ()
	{
		#  key and comma flag local variables
		echo -n [
		eval "typeset k$1 c$1=
		for k$1 in \"\${!$1[@]}\";do
			[ -z \"\$c$1\" ] || echo -n ,
			c$1=1
			echo -n \"\${$1[\$k$1]}\"
		done"
		echo -n ]
	}
	
	jkparseContinuousArrayToJson()
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
	
	jkparseQContinuousArrayToJson()
	{
		#  index local variable
		echo -n [
		eval "typeset i$1=-1
		while [ \$((++i$1)) -lt \${#$1[@]} ];do
			[ \"\$i$1\" = 0 ] || echo -n ,
			echo -n \"\${$1[\$i$1]}\"
		done"
		echo -n ]
	}
fi
