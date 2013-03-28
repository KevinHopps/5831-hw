#ifndef _kutils_h_
#define _kutils_h_

// Parses str using whitespace and stores the start
// of each non-whitespace string as a pointer in
// argv[] and returns the count.
//
// For example, "this is a test" will return 4,
// with the blanks turned into nulls and each
// argv[i] pointing to a word.
//
// Be sure argv[] is long enough!
//
extern int make_argv(char** argv, char* str);

// Returns non-zero on match.
//
int matchIgnoreCase(const char* s1, const char* s2, int maxLen);

#endif // #ifndef _kutils_h_
