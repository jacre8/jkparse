//  jkparse
//  JSON parser for shell scripts that utilizes the (associative) array capabilities of ksh and
// similar shells.
//  Copyright (C) 2022 Jason Hinsch
//  License: GPLv2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
//  See https://github.com/jacre8/jkparse for the latest version and documentation

#define JKPRINT_VERSION_STRING "4"

//  Compile with: gcc -O2 -o jkparse jkparse.c -ljson-c
//  If it is desired to use a shell's builtin printf rather than coreutils' or another standalone
// printf, declare USE_SHELL_PRINTF with the path to the shell as a constant string.
// E.g: gcc -D 'USE_SHELL_PRINTF="/bin/ksh"' -O2 -o jkparse jkparse.c -ljson-c

#ifdef USE_SHELL_PRINTF
	#ifndef SHELL_BASENAME
		#define SHELL_BASENAME "ksh"
	#endif
	#define PRINTF_EXECUTABLE USE_SHELL_PRINTF
#elif ! defined(PRINTF_EXECUTABLE)
	#define PRINTF_EXECUTABLE "/usr/bin/printf"
#endif
//#define TRIM_ARRAY_LEADING_SPACE  // for vanity's sake
//  At least as early as zsh v5.7.1, the workarounds with the following option are uneceesary.
// These issues may even have been fixed by v5.5.
//#define WORKAROUND_OLD_ZSH_SUBSCRIPT_BUGS

#define _GNU_SOURCE // for fputs_unlocked
//  It is recommended that a symlink be created at json-c/json.h if it is located somewhere else.
#ifdef __has_include
	#if __has_include(<json-c/json.h>)
		#include <json-c/json.h>
	#else
		#include <json/json.h>
	#endif
#else
	#include <json-c/json.h>
#endif
#include <getopt.h>
#include <stdio.h>
#include <stdio_ext.h> // __fsetlocking()
#include <stdlib.h>
#include <sysexits.h>
#include <sys/wait.h>
#include <unistd.h>

static void putShEscapedString(const char * str)
{
	fflush_unlocked(stdout);
	pid_t pid = vfork();
	if(0 == pid) 
	{
	#ifdef USE_SHELL_PRINTF
			static const char * const shellBasename = SHELL_BASENAME;
			execv(USE_SHELL_PRINTF, (char * const []){(char *)shellBasename, "-c", "printf %q \"$1\"",
					(char *)shellBasename, (char *)str, NULL});
	#else
			//execv("/usr/bin/env", (char * const []){"env", "printf", "%q", (char *)str, NULL});
			execv(PRINTF_EXECUTABLE, (char * const []){"printf", "%q", (char *)str, NULL});
	#endif
		_exit(EX_OSFILE);
	}
	if(-1 == pid)
		exit(EX_OSERR);
	int status;
	waitpid(pid, &status, 0);
	if(status)
	{
		fprintf(stderr, "\nUnable to execute " PRINTF_EXECUTABLE "\n");
		exit(WIFEXITED(status) ? WEXITSTATUS(status) : (WIFSIGNALED(status) ? 128 + status : status));
	}
}



static void putQuotedString(const char * str)
{
	fputs_unlocked("\\\"", stdout);
	putShEscapedString(str);
	fputs_unlocked("\\\"", stdout);
}


static void valPrintWithQuotedStrings(json_object * val)
{
	(json_type_string == json_object_get_type(val) ? putQuotedString :
		putShEscapedString)(json_object_get_string(val));
}


static void valPrintWithoutQuotedStrings(json_object * val)
{
	putShEscapedString(json_object_get_string(val));
}


int main(int argc, char **argv)
{
	char *arrayVarName = "";
	char *objVarName = "JSON_OBJ";
	char *typeVarName = "JSON_TYPE";
	char *emptyKey = "$'\\1'";
	const char * declareStr = "typeset";
	int quoteStrings = 0;
	int unsetVars = 0;
	__fsetlocking(stdout, FSETLOCKING_BYCALLER);
	{
		int currentoption;
		static const struct option longopts[] = {
			// {.name, .has_arg, .flag, .val}
			{"help", no_argument, NULL, '!'},
			{"array-var", required_argument, NULL, 'a'},
			{"empty-key", required_argument, NULL, 'e'},
			{"local-declarations", no_argument, NULL, 'l'},
			{"obj-var", required_argument, NULL, 'o'},
			{"quote-strings", no_argument, NULL, 'q'},
			{"short-version", no_argument, NULL, 'v'},
			{"type-var", required_argument, NULL, 't'},
			{"unset-vars", no_argument, NULL, 'u'},
			{"version", no_argument, NULL, 'V'},
			{0, 0, 0, 0}
		};
		opterr = 0;
		while( -1 != (currentoption = getopt_long(argc, argv, "a:e:lo:qt:uv", longopts, &currentoption)) )
		{
			switch(currentoption)
			{
			case '!':
				puts(
					"Typical usage: . <(jkparse [OPTIONS...] [JSON])\n"
					"  Parse JSON and return shell code for variable initialization based on the\n"
					"JSON contents.  This will read the JSON to parse either from an argument or, if\n"
					"that is not present, from stdin.  The returned shell code can be processed by\n"
					"bash v4+, ksh93, or zsh v5.5+.  Two variable declarations are output:\n"
					"  JSON_TYPE - this is a single character describing the detected type of the\n"
					"JSON argument.  This character is the first character for one of the following\n"
					"types: null, boolean, int, double, string, array, or object.  The type will be\n"
					"null if JSON cannot be represented by one of the other types.\n"
					"  JSON_OBJ - this is the parsed result.  It is formatted based on JSON_TYPE as\n"
					"one of the following:\n"
					" null - empty string\n"
					" boolean - string containing either 'true' or 'false'\n"
					" int, double - decimal string value\n"
					" string - string value without quotes\n"
					" array - array containing a string representation of each member\n"
					" object - associative array containing a string representation of each member\n"
					"  When JSON is either an array or an object type, non-string typed members of\n"
					"the output array can be fed back through this program for further processing.\n"
					"String typed members can also be fed back through if the --quote-strings option\n"
					"is specified when the array is generated.\n"
					"  There is no special handling for duplicated keys in objects.  When there are\n"
					"duplicate keys, multiple assignments will be output in the order that the keys\n"
					"appear in the original JSON.\n"
					"  This does not stream process the input when reading from stdin; if ever input\n"
					"stream processing were implemented, this may output the variable declarations\n"
					"twice.\n"
					"\n"
					"OPTIONS:\n"
					" -a, --array-var=JSON_OBJ_TYPES\n"
					"    When JSON_OBJ is either an array or object type, declare a third variable,\n"
					"  named JSON_OBJ_TYPES, that contains an array or associative array containing\n"
					"  characters corresponding to the types of the array or object's members,\n"
					"  respectively.  The characters in this array are the same characters used in\n"
					"  JSON_TYPE.  When JSON_OBJ_TYPES is an empty string, which is the default,\n"
					"  this variable declaration is omitted from the output\n"
					" -e, --empty-key=EMPTY_KEY\n"
					"    Empty keys are valid in JSON but not in shell script arrays.  Specify a\n"
					"  string to replace empty keys with.  The default is \"$'\\1'\".  This value\n"
					"  must be suitable for shell use.  No verification or substitution in output is\n"
					"  made for a non-empty value that is specified here.  An empty value is invalid\n"
					" -l, --local-declarations\n"
					"    Declare variables using the local keyword rather than the default, typeset\n"
					" -o, --obj-var=JSON_OBJ\n"
					"    Specify a variable name for JSON_OBJ other than the default, JSON_OBJ.\n"
					"  If blank, the object and array variables will be omitted from the output\n"
					" -q, --quote-strings\n"
					"    Include quotations around output string values so that they can be fed back\n"
					"  through this program with corresponding type detection\n"
					" -t, --type-var=JSON_TYPE\n"
					"    Specify a variable name for JSON_TYPE other than the default, JSON_TYPE.\n"
					"  If blank, the type variable will be omitted from the output\n"
					" -u, --unset-vars\n"
					"    Output commands to unset JSON_OBJ and, if defined, JSON_OBJ_TYPES, before\n"
					"  outputting their new declarations.  This permits re-using the same variable\n"
					"  names, and using JSON_OBJ for both input and output, while transversing an\n"
					"  object\n"
					" -v, --short-version\n"
					"    Output just the version number and exit\n"
					" --help\n"
					"    This help screen\n"
					" --version\n"
					"    Output version, copyright, and build options, then exit\n"
					"  Any non-empty variable name specified via an option will appear verbatim in\n"
					"the output without additional verification."
				);
				return EXIT_SUCCESS;
			case 'a':
				arrayVarName = optarg;
				break;
			case 'e':
				if(! *(emptyKey = optarg))
				{
					fprintf(stderr, "An empty argument for --empty-key is invalid\n");
					return EX_USAGE;
				}
				break;
			case 'l':
				declareStr = "local";
				break;
			case 'o':
				objVarName = optarg;
				break;
			case 'q':
				quoteStrings = 1;
				break;
			case 't':
				typeVarName = optarg;
				break;
			case 'u':
				unsetVars = 1;
				break;
			case 'v':
				fputs_unlocked(JKPRINT_VERSION_STRING "\n", stdout);
				return EXIT_SUCCESS;
				break;
			case 'V':
				fputs_unlocked("jkparse version " JKPRINT_VERSION_STRING
					"\nCopyright (C) 2022 Jason Hinsch\n"
					"License: GPLv2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n"
					"See https://github.com/jacre8/jkparse for the latest version and documentation\n"
					"Compiled with:\n"
					#ifndef USE_SHELL_PRINTF
						" PRINTF_EXECUTABLE=\"" PRINTF_EXECUTABLE "\"\n"
					#endif
					#ifdef TRIM_ARRAY_LEADING_SPACE
						" TRIM_ARRAY_LEADING_SPACE\n"
					#endif
					#ifdef USE_SHELL_PRINTF
						" USE_SHELL_PRINTF=\"" USE_SHELL_PRINTF "\"\n"
					#endif
					#ifdef WORKAROUND_OLD_ZSH_SUBSCRIPT_BUGS
						" WORKAROUND_OLD_ZSH_SUBSCRIPT_BUGS\n"
					#endif
					, stdout
				);
				return EXIT_SUCCESS;
				break;
			case '?':
				fprintf(stderr, "Invalid option: -%c\n", optopt);
				#if defined(__GNUC__) && __GNUC__ >= 7
					__attribute__((fallthrough));
				#endif
			default:
				return EX_USAGE;
			}
		}
	}
	json_object * obj;
	if(optind < argc)
	{
		//  The JSON object is an argument
		obj = json_tokener_parse(argv[optind]);
	}
	else
	{
		//  Read the JSON object from stdin
		char * input;
		//  Not only is this inefficient with memory, there is no limit on size here...
		scanf("%m[\x01-\xFF]", &input);
		obj = json_tokener_parse(input);
		free(input);
	}
	json_type type = json_object_get_type(obj);
	if(0 == *objVarName)
	{
		if(*typeVarName)
		{
			printf("%s %s=%c\n", declareStr, typeVarName, *json_type_to_name(type));
		}
		return 0;
	}
	static const char * associativeDeclareType = "-A ";
	static const char * arrayDeclareType = "-a ";
	void printTypeAndBeginObjWithType(const char * declareType)
	{
		//  Include a ';' between commands so that this can also be used with eval
		if(*typeVarName)
			printf("%s %s=%c;", declareStr, typeVarName, *json_type_to_name(type));
		if(unsetVars)
			printf("unset %s;", objVarName);
		printf("%s %s%s=", declareStr, declareType, objVarName);
	}
	void printTypeAndBeginObj(void)
	{
		printTypeAndBeginObjWithType("");
	}
	void printArrayClosureAndBeginArrayVar(const char * declareType)
	{
		//  Include a ';' between commands so that this can also be used with eval
		fputs_unlocked(");", stdout);
		if(unsetVars)
			printf("unset %s;", arrayVarName);
		printf("%s %s%s=(", declareStr, declareType, arrayVarName);
	}
	void printObject(void (*valuePrintFunction)(json_object *))
	{
	#ifdef TRIM_ARRAY_LEADING_SPACE
		const char * openBracketStr = "[";
		json_object_object_foreach(obj, key, val)
		{
			fputs_unlocked(openBracketStr, stdout);
			openBracketStr = " [";
	#else
		json_object_object_foreach(obj, key, val)
		{
			fputs_unlocked(" [", stdout);
	#endif
			if(*key)
			{
				int keyVal = *key;
				//  Escape the following characters, newline, tab, and space in the key output:
				//  !"$'();<>[\]`|  
				//  () and whitespace need to be escaped for zsh.  Excaping these makes no
				// difference in bash or ksh.  '#' could also be escaped for better ASCII
				// grouping in bash and ksh, but not in zsh
				char * segmentStart = key;
			#ifdef WORKAROUND_OLD_ZSH_SUBSCRIPT_BUGS
				//  Old zsh versions are particularly finicky about ", ', ;, <, >, and | characters in array
				// subscripts.  It is not enough to escape these characters - they must come from either
				// a variable or command substitution.  $'|' or $'\x7c', for example, do not work.
				while(
					(
						'"' != keyVal && '\'' != keyVal && ';' != keyVal &&
						'<' != keyVal && '>' != keyVal && '|' != keyVal
					) ? (
						(
							'\t' != keyVal && '\n' != keyVal && ' ' != keyVal && '!' != keyVal &&
							 '$' != keyVal && '(' != keyVal && ')' != keyVal &&
							//  This range includes '\\'
							('[' > keyVal || ']' < keyVal) && '`' != keyVal
						) ? (
							(keyVal = *(++key)) || ({
								fwrite_unlocked(segmentStart, key - segmentStart, 1, stdout);
								0;
							})
						) : ({
							//  Output all characters proceding the character needing escaping, and
							// then output the escaped character
							fwrite_unlocked(segmentStart, key - segmentStart, 1, stdout);
							putc_unlocked('\\', stdout);
							putc_unlocked(keyVal, stdout);
							keyVal = *(segmentStart = ++key);
						})
					) : ({
						//  This character requires substitution in zsh.  Output the rest
						// of the key as a command substitution
						fputs_unlocked("$(echo ", stdout);
						putShEscapedString(segmentStart);
						putc_unlocked(')', stdout);
						0;
					})
				);
			#else
				while(
					(
						'\t' != keyVal && '\n' != keyVal &&
						//  This range includes '!':
						(' ' > keyVal || '"' < keyVal) &&
						'$' != keyVal &&
						//  This range includes '(':
						('\'' > keyVal || ')' < keyVal) &&
						';' != keyVal && '<' != keyVal && '>' != keyVal && 
						//  This range includes '\\':
						('[' > keyVal || ']' < keyVal) &&
						'`' != keyVal && '|' != keyVal
					) ? (
						(keyVal = *(++key)) || ({
							fwrite_unlocked(segmentStart, key - segmentStart, 1, stdout);
							0;
						})
					) : ({
						//  Output all characters proceding the character needing escaping, and
						// then output the escaped character
						fwrite_unlocked(segmentStart, key - segmentStart, 1, stdout);
						putc_unlocked('\\', stdout);
						putc_unlocked(keyVal, stdout);
						keyVal = *(segmentStart = ++key);
					})
				);
			#endif
			}
			else
			{
				//  Empty keys are valid in JSON but not in shell scripts.
				// Output emptyKey as the key.
				fputs_unlocked(emptyKey, stdout);
			}
			fputs_unlocked("]=", stdout);
			valuePrintFunction(val);
		}
	}
	switch(type)
	{
	case json_type_null: // (i.e. obj == NULL),
		printTypeAndBeginObj();
		putc_unlocked('\n', stdout);
		break;
	case json_type_boolean:
	case json_type_double:
	case json_type_int:
		printTypeAndBeginObj();
		puts(json_object_get_string(obj));
		break;
	case json_type_object:
		printTypeAndBeginObjWithType(associativeDeclareType);
		putc_unlocked('(', stdout);
		{
			printObject(quoteStrings ? valPrintWithQuotedStrings : valPrintWithoutQuotedStrings);
		}
		if(*arrayVarName)
		{
			void valTypePrint(json_object * val)
			{
				putc_unlocked(*json_type_to_name(json_object_get_type(val)), stdout);
			}
			printArrayClosureAndBeginArrayVar(associativeDeclareType);
			printObject(valTypePrint);
		}
		puts(")");
		break;
	case json_type_array:
		printTypeAndBeginObjWithType(arrayDeclareType);
		putc_unlocked('(', stdout);
		{
			void (*valPrint)(json_object *) = quoteStrings
				? valPrintWithQuotedStrings : valPrintWithoutQuotedStrings;
			int arrayLength = json_object_array_length(obj);
			for(int index = 0; index < arrayLength; index++)
			{
			#ifdef TRIM_ARRAY_LEADING_SPACE
				if(index)
			#endif
					putc_unlocked(' ', stdout);
				json_object * objAtIndex;
				if(NULL == (objAtIndex = json_object_array_get_idx(obj, index)))
					fputs_unlocked("''", stdout);
				else
					valPrint(objAtIndex);
			}
			if(*arrayVarName)
			{
				printArrayClosureAndBeginArrayVar(arrayDeclareType);
				for(int index = 0; index < arrayLength; index++)
				{
				#ifdef TRIM_ARRAY_LEADING_SPACE
					if(index)
				#endif
						putc_unlocked(' ', stdout);
					putc_unlocked(*json_type_to_name(json_object_get_type(
						json_object_array_get_idx(obj, index))), stdout);
				}
			}
		}
		puts(")");
		break;
	case json_type_string:
		printTypeAndBeginObj();
		(quoteStrings ? putQuotedString : putShEscapedString)(json_object_get_string(obj));
		putc_unlocked('\n', stdout);
		break;
	}
	//json_object_put(obj);
	return 0;
}
