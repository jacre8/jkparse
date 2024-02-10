#  Functions for converting output from jkparse, presumably after
# modifications, back to JSON.  This is intended to be sourced, although there
# are no license restrictions to prevent copying the definitions within this
# file into another work.

#  Copyright 2023-2024 Jason Hinsch
#  No restrictions on use; Public Domain.

#  The function definitions herein are intended to be compatible with bash,
# ksh, and zsh.  The output of these functions is not pretty, but it is
# functional.  The output does not include a trailing newline.

#  The following functions are defined here:

# jkparseOutputToJson()
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

jkparseOutputToJson ()
{
	case $1 in
		b|d|i|q) eval echo -n \"\$"$2"\";;
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
				*) echo -n :\"\${$1[\$k$1$2]}\";;
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
				*) echo -n \"\${$1[\$i$1$2]}\";;
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
