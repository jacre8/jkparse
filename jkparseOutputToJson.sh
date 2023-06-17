#  Functions for converting output from jkparse, after modifications, back to
# JSON.  This is intended to be sourced.

#  Copyright 2023 Jason Hinsch
#  No restrictions on use; Public Domain.

#  Take the array variable whose name is in $1, with types in the array
# variable whose name is in $2, and print it as JSON.  This function's output
# may be incorrect if either name is eyk or oamcm.  $2 can be the name of an
# empty array, in which case it will be assumed that the jkparse output was
# generated using the -q option.
function jkparseArrayToJson
{
	typeset eyk=-1 oamcm=
	echo -n [
	eval 'while [ $((++eyk)) -lt ${#'"$1"'[@]} ];do
		[ -z "$oamcm" ] || echo -n ,
		oamcm=1
		if [ "${'"$2"'[$eyk]}" = s ];then
			jkparse -qs "${'"$1"'[$eyk]}"
		else
			echo -n "${'"$1"'[$eyk]}"
		fi
	done'
	echo -n ]
}

#  Take the associative array variable whose name is in $1, with types in
# the associative array variable whose name is in $2, and print it as JSON.
# This function may spew vile inflammatories and curse your children if
# either name is eyk or oamcm.  $2 can be the name of an empty associative
# array, in which case it will be assumed that the jkparse output was
# generated using the -q option.
function jkparseObjToJson
{
	typeset eyk oamcm=
	echo -n {
	eval 'for eyk in "${!'"$1"'[@]}";do
		[ -z "$oamcm" ] || echo -n ,
		oamcm=1
		echo -n \""$eyk"\":
		if [ "${'"$2"'[$eyk]}" = s ];then
			jkparse -qs "${'"$1"'[$eyk]}"
		else
			echo -n "${'"$1"'[$eyk]}"
		fi
	done'
	echo -n }
}

#  $1 = $JSON_TYPE
#  $2 = name of JSON_OBJ
#  $3 = name of JSON_OBJ_TYPES
#  Take variables created using jkparse and output JSON.  This function will
# sully your core if either name is eyk or oamcm and $1 = a or o.  $3 can be
# the name of an empty associative array, in which case it will be assumed
# that the jkparse output was generated using the -q option when $1 = a or o.
jkparseOutputToJson ()
{
	case $1 in
		b|d|i|q) eval echo -n \"\$"$2"\" ;;
		a) jkparseArrayToJson "$2" "$3" ;;
		o) jkparseObjToJson "$2" "$3" ;;
		s) eval jkparse -qs \"\$"$2"\" ;;
	esac
}
