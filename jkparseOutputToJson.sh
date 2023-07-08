#  Functions for converting output from jkparse, after modifications, back to
# JSON.  This is intended to be sourced.

#  Copyright 2023 Jason Hinsch
#  No restrictions on use; Public Domain.

#  Take the array variable whose name is in $1, with types in the array
# variable whose name is in $2, and print it as JSON.  $2 can be the name of an
# empty array, in which case it will be assumed that the jkparse output was
# generated using the -q option.
#  This version handles sparse arrays (arrays that are missing indices).
function jkparseArrayToJson
{
	#  To avoid any possible name conflict, the key and comma flag variable
	# names incorporate both argument names.
	echo -n [
	eval "typeset k$1$2 c$2$1=
	for k$1$2 in \"\${!$1[@]}\";do
		[ -z \"\$c$2$1\" ] || echo -n ,
		c$2$1=1
		case \"\${$2[\$k$1$2]}\" in
			s) jkparse -qs \"\${$1[\$k$1$2]}\";;
			n) echo -n null;;
			*) echo -n \"\${$1[\$k$1$2]}\";;
		esac
	done"
	echo -n ]
}

#  This may be faster than jkparseArrayToJson(), but it does not handle missing
# elements gracefully.
function jkparseContinuousArrayToJson
{
	#  To avoid any possible name conflict, the index variable name
	# incorporates both argument names.
	echo -n [
	eval "typeset i$1$2=-1
	while [ \$((++i$1$2)) -lt \${#$1[@]} ];do
		[ \"\$i$1$2\" = 0 ] || echo -n ,
		case \"\${$2[\$i$1$2]}\" in
			s) jkparse -qs \"\${$1[\$i$1$2]}\";;
			n) echo -n null;;
			*) echo -n \"\${$1[\$i$1$2]}\";;
		esac
	done"
	echo -n ]
}

#  Take the associative array variable whose name is in $1, with types in
# the associative array variable whose name is in $2, and print it as JSON.
# $2 can be the name of an empty associative array, in which case it will be
# assumed that the jkparse output was generated using the -q option.  If the
# optional argument $3 is defined, keys that match it will be treated as
# empty strings in the output.  If $3 is undefined, keys matching the string
# $'\1' will be treated as empty key strings.
function jkparseObjToJson
{
	#  To avoid any possible name conflict, the key and comma flag variable
	# names incorporate both argument names.
	echo -n {
	eval "typeset k$1$2 c$2$1=
	for k$1$2 in \"\${!$1[@]}\";do
		[ -z \"\$c$2$1\" ] || echo -n ,
		c$2$1=1
		if [ \"\$k$1$2\" = \"\${3-$'\1'}\" ];then
			printf '\"\"'
		else
			jkparse -qs \"\$k$1$2\"
		fi
		case \"\${$2[\$k$1$2]}\" in
			s) echo -n :
			   jkparse -qs \"\${$1[\$k$1$2]}\";;
			n) echo -n :null;;
			*) echo -n :\"\${$1[\$k$1$2]}\";;
		esac
	done"
	echo -n }
}

#  $1 = $JSON_TYPE
#  $2 = name of JSON_OBJ
#  $3 = name of JSON_OBJ_TYPES
#  $4 = object empty key string (the default, if undefined, is $'\1')
#  Take variables created using jkparse and output JSON.  $3 can be the name of
# an empty associative array, in which case it will be assumed that the jkparse
# output was generated using the -q option when $1 = a or o.  If the optional
# argument $3 is defined, object keys that match it will be treated as empty
# strings in the output.  If $3 is undefined, keys matching the string $'\1'
# will be treated as empty key strings.
jkparseOutputToJson ()
{
	case $1 in
		b|d|i|q) eval echo -n \"\$"$2"\";;
		a) jkparseArrayToJson "$2" "$3";;
		o) eval jkparseObjToJson '$2' '$3' \"\${4-$'\1'}\";;
		s) eval jkparse -qs \"\$"$2"\";;
		n) echo -n null;;
	esac
}
