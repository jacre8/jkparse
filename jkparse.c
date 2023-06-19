//  jkparse
//  JSON parser for shell scripts that utilizes the (associative) array capabilities of ksh and
// similar shells.

#define JKPRINT_VERSION_STRING "7"
#define JKPRINT_VERSION_STRING_LONG "jkparse version " JKPRINT_VERSION_STRING \
"\nCopyright (C) 2022-2023 Jason Hinsch\n" \
"License: GPLv2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n" \
"See https://github.com/jacre8/jkparse for the latest version and documentation\n"

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
// These issues may even have been fixed by v5.5.  These issues were observed in v5.3.1.
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
#include <string.h>
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


static void putJsonEscapedString(void (* putf)(const char *), const char * str,
	int withSurroundingQuotes)
{
	//  For proper JSON formatting, use the json-c library to escape the string's characters.  To
	// get the library to do this, add it to a json array object and remove the array formatting.
	int paddingOffset = 1 + (! withSurroundingQuotes);
	json_object *jarray = json_object_new_array();
	json_object_array_add(jarray, json_object_new_string(str));
	//  json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PLAIN) will return the string
	// as '["str"]'.
	//  json_object_to_json_string_length() is only present in json-c v0.13, or newer.  The
	// JSON_C_TO_STRING_PRETTY_TAB macro was also introduced in v0.13.
	#ifdef JSON_C_TO_STRING_PRETTY_TAB
		size_t termCharIndex;
		char * arrayedStr = (char *)json_object_to_json_string_length(jarray,
			JSON_C_TO_STRING_PLAIN, &termCharIndex);
		termCharIndex -= paddingOffset;
	#else
		char * arrayedStr = (char *)json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PLAIN);
		size_t termCharIndex = strlen(arrayedStr) - paddingOffset;
	#endif
	// The leading '[' is easily accounted for, but we must come back and strip the trailing ']'.
	// Albeit bad form, it does work to terminate it in place rather than copy the whole string.
	arrayedStr[termCharIndex] = 0;
	putf(arrayedStr + paddingOffset);
	//  To avoid potential issues without making assumptions about json-c's implementation,
	// put the trailing ']' back before freeing jarray
	arrayedStr[termCharIndex] = ']';
	json_object_put(jarray);
}


static void putShEscapedAndQuotedJsonString(const char * str)
{
	putJsonEscapedString(putShEscapedString, str, 1);
}


//  For use with putJsonEscapedString()
static void stdPutf(const char * str)
{
	fputs_unlocked(str, stdout);
}

static void valPrintWithQuotedStrings(json_object * val)
{
	if(val)
		(json_type_string == json_object_get_type(val) ? putShEscapedAndQuotedJsonString :
			putShEscapedString)(json_object_get_string(val));
	else
		fputs_unlocked("null", stdout);
	/*enum json_type type = json_object_get_type(val);
	if(json_type_null == type)
		fputs_unlocked("null", stdout);
	else
		(json_type_string == type ? putShEscapedAndQuotedJsonString :
			putShEscapedString)(json_object_get_string(val));
	*/
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
	int verbose = 0;
	__fsetlocking(stdout, FSETLOCKING_BYCALLER);
	{
		int stringify = 0;
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
			{"stringify", no_argument, NULL, 's'},
			{"type-var", required_argument, NULL, 't'},
			{"unset-vars", no_argument, NULL, 'u'},
			{"verbose", no_argument, NULL, 'V'},
			{"version", no_argument, NULL, '@'},
			{0, 0, 0, 0}
		};
		opterr = 0;
		while( -1 != (currentoption = getopt_long(argc, argv, "a:e:lo:qst:uvV", longopts, &currentoption)) )
		{
			switch(currentoption)
			{
			case '!':
				puts(
					"Typical usage: . <(jkparse [OPTIONS...] [JSON])\n"
					"  Parse JSON and return shell code for variable initialization based on the\n"
					"JSON contents.  This will read the JSON to parse either from the first non-\n"
					"option argument or, if one is not present, from stdin.  The returned shell code\n"
					"can be processed by bash v4+, ksh93, or zsh v5.5+.  Two variable declarations\n"
					"are output:\n"
					"  JSON_TYPE - this is a single character describing the detected type of the\n"
					"JSON argument.  This character is the first character for one of the following\n"
					"types: null, boolean, int, double, string, array, or object.  The type will be\n"
					"null if JSON cannot be represented by one of the other types.\n"
					"  JSON_OBJ - this is the parsed result.  It is formatted based on JSON_TYPE as\n"
					"one of the following:\n"
					" null - empty string\n"
					" boolean - string containing either 'true' or 'false'\n"
					" int, double - decimal string value\n"
					" string - string value without quotes or JSON escapes\n"
					" array - array containing a string representation of each member\n"
					" object - associative array containing a string representation of each member\n"
					"  Output values whose type is neither string nor null can always be fed back\n"
					"through this program, without modification, for further processing.  String and\n"
					"null typed output values can also be fed back through, without modification, if\n"
					"the --quote-strings option is specified when the output is generated.\n"
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
					"    Include quotations around output string values, and escape as necessary to\n"
					"  generate valid JSON, so that they can be fed back through this program with\n"
					"  corresponding type detection.  For the sake of subsequent encoding, the type\n"
					"  indicator for strings will be 'q' with this option instead of 's'.  With this\n"
					"  option, null values will also be explictily output as null, rather than as\n"
					"  empty strings\n"
					" -s, --stringify\n"
					"    Take the input and output it escaped as a JSON string, without surrounding\n"
					"  quotes, whitespace, or shell escapes.  This is a formatting-only function\n"
					"  that is intended for use in constructing JSON text.  The only other option\n"
					"  that this may be logically combined with is -q, which only adds surrounding\n"
					"  quotes in the output when combined\n"
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
					" -V, --verbose\n"
					"    If there is a parse error, output a descriptive message to stderr\n"
					" --help\n"
					"    This help screen\n"
					" --version\n"
					"    Output version, copyright, and build options, then exit\n"
					"  Any non-empty variable name specified via an option will appear verbatim in\n"
					"the output without additional verification.  Additional options for variable\n"
					"declaration may be specified in the -a, -o, and -t option arguments.  E.g.,\n"
					"-o '-g JSON_OBJ' will promote the scope of the object's declaration in BASH."
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
			case 's':
				stringify = 1;
				break;
			case 'u':
				unsetVars = 1;
				break;
			case 'v':
				fputs_unlocked(JKPRINT_VERSION_STRING "\n", stdout);
				return EXIT_SUCCESS;
				break;
			case 'V':
				verbose = 1;
				break;
			case '@':
				fputs_unlocked(JKPRINT_VERSION_STRING_LONG
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
		if(stringify)
		{
			char * input;
			if(optind < argc)
				input = argv[optind];
			else
				//  Not only is this inefficient with memory, there is no limit on size here...
				scanf("%m[\x01-\xFF]", &input);
			putJsonEscapedString(stdPutf, input, quoteStrings);
			return 0;
		}
	}
	json_object * obj;
	enum json_tokener_error parseError = 0;
	if(optind < argc)
	{
		//  The JSON object is an argument
		//obj = json_tokener_parse(argv[optind]);
		obj = json_tokener_parse_verbose(argv[optind], &parseError);
	}
	else
	{
		//  Read the JSON object from stdin
		struct json_tokener *tok = json_tokener_new();
		if(! tok)
			return EX_OSERR;
		do
		{
			char inputBuffer[65536];
			ssize_t readRc = read(0, inputBuffer, sizeof(inputBuffer));
			if(0 >= readRc)
			{
				parseError = json_tokener_error_parse_eof;
				break;
			}
			obj = json_tokener_parse_ex(tok, inputBuffer, readRc);
		}
		while(json_tokener_continue == (parseError = json_tokener_get_error(tok)));
		json_tokener_free(tok);
	}
	if(parseError && verbose)
		fprintf(stderr, "Error parsing JSON: %s\n", json_tokener_error_desc(parseError));
	json_type type = json_object_get_type(obj);
	if(0 == *objVarName)
	{
		if(*typeVarName)
		{
			char typeChar = *json_type_to_name(type);
			printf("%s %s=%c\n", declareStr, typeVarName,
				's' == typeChar && quoteStrings ? 'q' : typeChar);
		}
		return 0;
	}
	static const char * associativeDeclareType = "-A ";
	static const char * arrayDeclareType = "-a ";
	char objTypeChar;
	void printTypeAndBeginObjWithType(const char * declareType)
	{
		//  Include a ';' between commands so that this can also be used with eval
		if(*typeVarName)
			printf("%s %s=%c;", declareStr, typeVarName, objTypeChar);
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
							'\t' != keyVal && ' ' != keyVal && '!' != keyVal &&
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
						'\t' != keyVal &&
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
		objTypeChar = 'n';
		printTypeAndBeginObj();
		if(quoteStrings)
			fputs_unlocked("null\n", stdout);
		else
			putc_unlocked('\n', stdout);
		break;
	case json_type_boolean:
		objTypeChar = 'b';
		goto outputObjPlainly;
	case json_type_double:
		objTypeChar = 'd';
		goto outputObjPlainly;
	case json_type_int:
		objTypeChar = 'i';
	outputObjPlainly:
		//objTypeChar = *json_type_to_name(type);
		printTypeAndBeginObj();
		puts(json_object_get_string(obj));
		break;
	case json_type_object:
		objTypeChar = 'o';
		printTypeAndBeginObjWithType(associativeDeclareType);
		putc_unlocked('(', stdout);
		{
			void valPrintWithoutQuotedStrings(json_object * val)
			{
				putShEscapedString(json_object_get_string(val));
			}
			printObject(quoteStrings ? valPrintWithQuotedStrings : valPrintWithoutQuotedStrings);
		}
		if(*arrayVarName)
		{
			void valTypePrint(json_object * val)
			{
				putc_unlocked(*json_type_to_name(json_object_get_type(val)), stdout);
			}
			void valTypePrintWithQForStrings(json_object * val)
			{
				char typeChar = *json_type_to_name(json_object_get_type(val));
				putc_unlocked('s' == typeChar ? 'q' : typeChar, stdout);
			}
			printArrayClosureAndBeginArrayVar(associativeDeclareType);
			printObject(quoteStrings ? valTypePrintWithQForStrings : valTypePrint);
		}
		puts(")");
		break;
	case json_type_array:
		objTypeChar = 'a';
		printTypeAndBeginObjWithType(arrayDeclareType);
		putc_unlocked('(', stdout);
		{
			void arrayValPrintWithoutQuotedStrings(json_object * objAtIndex)
			{
				if(NULL == objAtIndex)
					fputs_unlocked("''", stdout);
				else
					putShEscapedString(json_object_get_string(objAtIndex));
			}
			void (*valPrint)(json_object *) = quoteStrings
				? valPrintWithQuotedStrings : arrayValPrintWithoutQuotedStrings;
			int arrayLength = json_object_array_length(obj);
			for(int index = 0; index < arrayLength; index++)
			{
			#ifdef TRIM_ARRAY_LEADING_SPACE
				if(index)
			#endif
					putc_unlocked(' ', stdout);
				valPrint(json_object_array_get_idx(obj, index));
			}
			if(*arrayVarName)
			{
				printArrayClosureAndBeginArrayVar(arrayDeclareType);
				if(quoteStrings)
				{
					for(int index = 0; index < arrayLength; index++)
					{
					#ifdef TRIM_ARRAY_LEADING_SPACE
						if(index)
					#endif
							putc_unlocked(' ', stdout);
						char typeChar = *json_type_to_name(json_object_get_type(
							json_object_array_get_idx(obj, index)));
						putc_unlocked('s' == typeChar ? 'q' : typeChar, stdout);
					}
				}
				else
				{
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
		}
		puts(")");
		break;
	case json_type_string:
		(
			quoteStrings ?
			({
				objTypeChar = 'q';
				printTypeAndBeginObj();
				putShEscapedAndQuotedJsonString;
			}) : ({
				objTypeChar = 's';
				printTypeAndBeginObj();
				putShEscapedString;
			})
		)(json_object_get_string(obj));
		putc_unlocked('\n', stdout);
		break;
	}
	//json_object_put(obj);
	return parseError;
}
