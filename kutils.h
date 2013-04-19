#ifndef _kutils_h_
#define _kutils_h_

#include "ktypes.h"

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
int make_argv(char** argv, char* str);

// This returns true if s1 and s2 match, ignoring case.
// Up to maxLen characters are compared.
//
bool matchIgnoreCase(const char* s1, const char* s2, int maxLen);

// ABS(X) returns the absolute value of X.
//
#define ABS(X) ((X) < 0 ? -(X) : (X))

// This returns -1, 0, or 1, according to whether
// X is negative, 0, or positive, respectively.
//
#define SGN(X) ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))

// This may be used to manipulate interrupt enabling.
// The setInterruptsEnabled() function will return the
// old value of interrupts enabled. These facilitate
// implementation of the BEGIN_ATOMIC and END_ATOMIC
// macros.
//
bool getInterruptsEnabled();
bool setInterruptsEnabled(bool enable); // returns old status

// If you have a sequence of code that needs to execute without the
// possibility of interrupts occurring, surround it with BEGIN_ATOMIC
// and END_ATOMIC. For example
//     BEGIN_ATOMIC
//         extern int fib_a, fib_b, fib_c;
//         int c = fib_a + fib_b;
//         fib_a = fib_b;
//         fib_b = fib_c;
//         fib_c = c;
//     END_ATOMIC
//
#define BEGIN_ATOMIC do { bool XXenabledXX_ = setInterruptsEnabled(false);
#define END_ATOMIC setInterruptsEnabled(XXenabledXX_); } while (false);

#endif // #ifndef _kutils_h_
