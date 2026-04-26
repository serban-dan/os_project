AI usage for the filter function:



* The prompt for the first function was as follows:

"typedef struct Record {

    int id; //4 bytes

    char inspector\[MAX\_NAME\_LEN]; //52 bytes

    float latitude; //4 bytes

    float longitude; //4 bytes

    char category\[MAX\_CAT\_LEN]; //20 bytes

    int severity; //4 bytes

    time\_t timestamp; //8 bytes

    char description\[MAX\_DESC\_LEN]; //112 bytes

} Record;



This is the record structure used for a city management program. You have to build a function int parse\_condition(const char \*input, char \*field, char \*op, char \*value); in c which splits a string "field:operator:value" into its three parts."



* Giving the AI more context such as actual lines of code that were already built helps it understand better what it actually needs to do and in the end we don't have to make as many changes. In fact I did not make any changes and the AI actually checked for null pointers and used strncpy for safety. at the last string it actually knew that the last '\\0' was already in the parsed string and used only strcpy for simplicty instead of adding it manually again.



\#include <stdio.h>

\#include <string.h>



/\*\*

&#x20;\* Parses a condition string formatted as "field:operator:value"

&#x20;\* \* @param input The input string to parse.

&#x20;\* @param field Buffer to store the extracted field name.

&#x20;\* @param op    Buffer to store the extracted operator.

&#x20;\* @param value Buffer to store the extracted value.

&#x20;\* @return      1 on success, 0 on failure (e.g., malformed string or null pointers).

&#x20;\*/

int parse\_condition(const char \*input, char \*field, char \*op, char \*value) {

&#x20;   // Null pointer safety check

&#x20;   if (!input || !field || !op || !value) {

&#x20;       return 0; 

&#x20;   }



&#x20;   // Find the first colon

&#x20;   const char \*first\_colon = strchr(input, ':');

&#x20;   if (!first\_colon) {

&#x20;       return 0; // Missing first colon

&#x20;   }



&#x20;   // Find the second colon, searching from the character after the first colon

&#x20;   const char \*second\_colon = strchr(first\_colon + 1, ':');

&#x20;   if (!second\_colon) {

&#x20;       return 0; // Missing second colon

&#x20;   }



&#x20;   // Calculate lengths of the field and operator substrings

&#x20;   size\_t field\_len = first\_colon - input;

&#x20;   size\_t op\_len = second\_colon - (first\_colon + 1);



&#x20;   // Extract the field and explicitly null-terminate

&#x20;   strncpy(field, input, field\_len);

&#x20;   field\[field\_len] = '\\0'; 



&#x20;   // Extract the operator and explicitly null-terminate

&#x20;   strncpy(op, first\_colon + 1, op\_len);

&#x20;   op\[op\_len] = '\\0';



&#x20;   // Extract the value (copies everything from the second colon to the null terminator)

&#x20;   strcpy(value, second\_colon + 1);



&#x20;   return 1;

}



* The second prompt was even simpler because the AI already had enough context from the previous prompt:

Now you will have to write a function int match\_condition(Record \*r, const char \*field, const char \*op, const char \*value); which returns 1 if the record satisfies the condition and 0 otherwise. The only supported fields are severity, category, inspector, timestamp. The supported operators are ==, !=, <, <=, >, >=.



\#include <stdio.h>

\#include <stdlib.h>

\#include <string.h>

\#include <time.h>



// Assuming your Record struct is defined here or included via a header

// typedef struct Record { ... } Record;



/\*\*

&#x20;\* Helper function to evaluate numeric comparisons.

&#x20;\* Casts values to long long to safely handle both int (severity) and time\_t (timestamp).

&#x20;\*/

static int compare\_numeric(long long rec\_val, long long cond\_val, const char \*op) {

&#x20;   if (strcmp(op, "==") == 0) return rec\_val == cond\_val;

&#x20;   if (strcmp(op, "!=") == 0) return rec\_val != cond\_val;

&#x20;   if (strcmp(op, "<")  == 0) return rec\_val < cond\_val;

&#x20;   if (strcmp(op, "<=") == 0) return rec\_val <= cond\_val;

&#x20;   if (strcmp(op, ">")  == 0) return rec\_val > cond\_val;

&#x20;   if (strcmp(op, ">=") == 0) return rec\_val >= cond\_val;

&#x20;   return 0; // Unknown operator

}



/\*\*

&#x20;\* Helper function to evaluate string comparisons.

&#x20;\* Uses standard lexicographical evaluation via strcmp.

&#x20;\*/

static int compare\_string(const char \*rec\_val, const char \*cond\_val, const char \*op) {

&#x20;   int cmp = strcmp(rec\_val, cond\_val);

&#x20;   if (strcmp(op, "==") == 0) return cmp == 0;

&#x20;   if (strcmp(op, "!=") == 0) return cmp != 0;

&#x20;   if (strcmp(op, "<")  == 0) return cmp < 0;

&#x20;   if (strcmp(op, "<=") == 0) return cmp <= 0;

&#x20;   if (strcmp(op, ">")  == 0) return cmp > 0;

&#x20;   if (strcmp(op, ">=") == 0) return cmp >= 0;

&#x20;   return 0; // Unknown operator

}



/\*\*

&#x20;\* Evaluates if a record matches a given condition.

&#x20;\* \* @param r     Pointer to the Record.

&#x20;\* @param field The field to evaluate (severity, category, inspector, timestamp).

&#x20;\* @param op    The comparison operator (==, !=, <, <=, >, >=).

&#x20;\* @param value The value to compare against, represented as a string.

&#x20;\* @return      1 if the record satisfies the condition, 0 otherwise.

&#x20;\*/

int match\_condition(Record \*r, const char \*field, const char \*op, const char \*value) {

&#x20;   if (!r || !field || !op || !value) {

&#x20;       return 0; // Safety guard

&#x20;   }



&#x20;   if (strcmp(field, "severity") == 0) {

&#x20;       int val = atoi(value);

&#x20;       return compare\_numeric((long long)r->severity, (long long)val, op);

&#x20;       

&#x20;   } else if (strcmp(field, "timestamp") == 0) {

&#x20;       // Using strtoll for safe conversion to 64-bit integers often used by time\_t

&#x20;       long long val = strtoll(value, NULL, 10);

&#x20;       return compare\_numeric((long long)r->timestamp, val, op);

&#x20;       

&#x20;   } else if (strcmp(field, "category") == 0) {

&#x20;       return compare\_string(r->category, value, op);

&#x20;       

&#x20;   } else if (strcmp(field, "inspector") == 0) {

&#x20;       return compare\_string(r->inspector, value, op);

&#x20;       

&#x20;   }



&#x20;   // Unsupported field

&#x20;   return 0; 

}



* Here we can see that it performed even better, it did not want to repeat itself for the sake of the user (clean and maintainable code is always better) so it used helper functions. Also by type casting numerical comparisons it makes the helper function handle them safer for 32 bit integers. The intresting part comes to edge cases which are harder for humans to think of, such as "inspector:>:A" which would print all inspectors whose names start with B-Z, so it used strcmp as the base for the compare\_string helper function. Finally the strtoll was used to safely convert the timestamp to a 64 bit int.

