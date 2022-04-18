//  jkparse
//  JSON parser for shell scripts that utilizes the array capabilities of ksh and similar shells.
//  Copyright (C) 2022 Jason Hinsch
//  License: GPLv2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>

//  Compile with: gcc -O2 -o jkparse jkparse.c -ljson-c
//  If it is desired to use a shell's builtin printf rather than coreutils' or another standalone
// printf, declare USE_SHELL_PRINTF with the path to the shell as a constant string.
// E.g: gcc -D 'USE_SHELL_PRINTF="/bin/ksh"' -O2 -o jkparse jkparse.c -ljson-c

#ifdef USE_SHELL_PRINTF
	#define PRINTF_EXECUTABLE USE_SHELL_PRINTF
#elif ! defined(PRINTF_EXECUTABLE)
	#define PRINTF_EXECUTABLE "/usr/bin/printf"
#endif

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
#ifdef USE_SHELL_PRINTF
	#include <string.h>
#endif
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
			static const char * const shellExec = USE_SHELL_PRINTF;
			char * name = basename(shellExec);
			execv(shellExec, (char * const []){name, "-c", "printf %q \"$1\"", name, (char *)str, NULL});
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

	
int main(int argc, char **argv)
{
	char *arrayVarName = "";
	char *objVarName = "JSON_OBJ";
	char *typeVarName = "JSON_TYPE";
	char *emptyKey = "$'\\1'";
	int quoteStrings = 0;
	__fsetlocking(stdout, FSETLOCKING_BYCALLER);
	{
		int currentoption;
		static const struct option longopts[] = {
			// {.name, .has_arg, .flag, .val}
			{"help", no_argument, NULL, '!'},
			{"array-var", required_argument, NULL, 'a'},
			{"empty-key", required_argument, NULL, 'e'},
			{"obj-var", required_argument, NULL, 'o'},
			{"quote-strings", no_argument, NULL, 'q'},
			{"type-var", required_argument, NULL, 't'},
			{"version", no_argument, NULL, 'v'},
			{0, 0, 0, 0}
		};
		opterr = 0;
		while( -1 != (currentoption = getopt_long(argc, argv, "a:e:o:qt:", longopts, &currentoption)) )
		{
			switch(currentoption)
			{
			case '!':
				puts(
					"Typical usage: . <(jkparse [OPTIONS...] [JSON])\n"
					"  Parse JSON and return shell code for variable initialization based on the\n"
					"JSON contents.  The returned shell code can be processed by bash v4+, ksh93,\n"
					"or zsh v5.5+.  Two variable declarations are output:\n"
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
					"is specified.\n"
					"\n"
					"OPTIONS:\n"
					" --help\n"
					"    This help screen\n"
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
					"  made for a non-empty value that is specified here\n"
					" -o, --obj-var=JSON_OBJ\n"
					"    Specify a variable name for JSON_OBJ other than the default, JSON_OBJ.\n"
					"  If blank, the object and array variables will be omitted from the output\n"
					" -q, --quote strings\n"
					"    Include quotations around output string values so that they can be fed back\n"
					"  through this program with corresponding type detection\n"
					" -t, --type-var=JSON_TYPE\n"
					"    Specify a variable name for JSON_TYPE other than the default, JSON_TYPE.\n"
					"  If blank, the type variable will be omitted from the output\n"
					" --version\n"
					"    Output the version information and exit\n"
					"  Any non-empty variable name specified via an option will appear verbatim in\n"
					"the output without additional verification.\n"
					"  There is no special handling for duplicated keys in objects.  When there are\n"
					"duplicate keys, multiple assignments will be output in the order that the keys\n"
					"appear in the original JSON.\n"
				);
				return EXIT_SUCCESS;
			case 'a':
				arrayVarName = optarg;
				break;
			case 'e':
				emptyKey = optarg;
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
			case 'v':
				fputs_unlocked("jkparse version 1\nCopyright (C) 2022 Jason Hinsch\n"
					"License: GPLv2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n"
					"Compiled with "
					#ifdef USE_SHELL_PRINTF
						"USE_SHELL_PRINTF=\"" USE_SHELL_PRINTF
					#else
						"PRINTF_EXECUTABLE=\"" PRINTF_EXECUTABLE
					#endif
					"\"\n", stdout
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
	if(optind >= argc)
	{
		fprintf(stderr, "JSON input missing\n");
		return EX_USAGE;
	}
	if(! *emptyKey)
	{
		fprintf(stderr, "The argument for --empty-key is blank\n");
		return EX_USAGE;
	}
	json_object * obj = json_tokener_parse(argv[optind]);
	json_type type = json_object_get_type(obj);
	if(0 == *objVarName)
	{
		if(*typeVarName)
		{
			printf("typeset %s=%c\n", typeVarName, *json_type_to_name(type));
		}
		return 0;
	}
	static const char * associativeDeclareType = "-A ";
	static const char * arrayDeclareType = "-a ";
	void printTypeAndBeginObjWithType(const char * declareType)
	{
		static const char * printTemplate = "typeset %s=%c\ntypeset %s%s=";
		if(*typeVarName)
			printf(printTemplate, typeVarName, *json_type_to_name(type),
				declareType, objVarName);
		else
			printf(printTemplate + 14, declareType, objVarName);
	}
	void printTypeAndBeginObj(void)
	{
		printTypeAndBeginObjWithType("");
	}
	void printArrayClosureAndBeginArrayVar(const char * declareType)
	{
		printf(")\ntypeset %s%s=(", declareType, arrayVarName);
	}
	void printObject(void (*valuePrintFunction)(json_object *))
	{
		int outputSeperator = 0;
		json_object_object_foreach(obj, key, val)
		{
			if(outputSeperator)
				fputs_unlocked(" [", stdout);
			else
			{
				outputSeperator = 1;
				putc_unlocked('[', stdout);
			}
			char * keyStart = key;
			//  Escape the following characters in the key output: !"$'();<>[\]`|
			//  () need to be escaped for zsh.  Excaping these makes no difference in bash or ksh.
			//  zsh is particularly finicky about ", ', ;, <, >, and | characters in array
			// subscripts.  It is not enough to escape these characters; they must come from either a
			// variable or command substitution.  $'|' or $'\x7c', for example, do not work.
			//  '#' could also be escaped for better ASCII grouping in bash and ksh, but not in zsh
			do
			{
				//  badZshChar will be set when a character which is problemetic for zsh is found
				int badZshChar = 0;
				char * segmentStart = key;
				int keyVal;
				while(
					(keyVal = *key) && (
						('"' != keyVal && '\'' != keyVal && ';' != keyVal && '<' != keyVal &&
							'>' != keyVal && '|' != keyVal) || ({ badZshChar = 1; 0; })
					) && '!' != keyVal  && '$' != keyVal && '(' != keyVal && ')' != keyVal &&
					//  This range includes '\\'
					('[' > keyVal || ']' < keyVal) && '`' != keyVal
				)
					key++;
				if(keyVal)
				{
					if(badZshChar)
					{
						//  Ouput the rest of the key as a command substitution
						fputs_unlocked("$(echo ", stdout);
						putShEscapedString(segmentStart);
						putc_unlocked(')', stdout);
						break;
					}
					else
					{
						//  Output all characters proceding the character needing escaping, and
						// then output the escaped character
						fwrite_unlocked(segmentStart, (key++) - segmentStart, 1, stdout);
						putc_unlocked('\\', stdout);
						putc_unlocked(keyVal, stdout);
					}
				}
				else
				{
					if(key == keyStart)
						//  Empty keys are valid in JSON but not in shell scripts.
						// Output emptyKey as the key.
						fputs_unlocked(emptyKey, stdout);
					else
						fwrite_unlocked(segmentStart, key - segmentStart, 1, stdout);
					break;
				}
			}
			while(*key);
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
			void valPrintWithQuotedStrings(json_object * val)
			{
				(json_type_string == json_object_get_type(val) ? putQuotedString :
					putShEscapedString)(json_object_get_string(val));
			}
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
			printArrayClosureAndBeginArrayVar(associativeDeclareType);
			printObject(valTypePrint);
		}
		puts(")");
		break;
	case json_type_array:
		printTypeAndBeginObjWithType(arrayDeclareType);
		putc_unlocked('(', stdout);
		int arrayLength = json_object_array_length(obj);
		for(int index = 0; index < arrayLength; index++)
		{
			if(index)
				putc_unlocked(' ', stdout);
			json_object * objAtIndex;
			if(NULL == (objAtIndex = json_object_array_get_idx(obj, index)))
				fputs_unlocked("''", stdout);
			else
			{
				(quoteStrings && json_type_string == json_object_get_type(objAtIndex) ? putQuotedString :
					putShEscapedString)(json_object_get_string(objAtIndex));
			}
		}
		if(*arrayVarName)
		{
			printArrayClosureAndBeginArrayVar(arrayDeclareType);
			for(int index = 0; index < arrayLength; index++)
			{
				if(index)
					putc_unlocked(' ', stdout);
				putc_unlocked(*json_type_to_name(json_object_get_type(json_object_array_get_idx(obj, index))), stdout);
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
