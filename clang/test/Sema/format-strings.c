// RUN: %clang_cc1 -fblocks -fsyntax-only -verify -Wformat-nonliteral -isystem %S/Inputs %s
// RUN: %clang_cc1 -fblocks -fsyntax-only -verify -Wformat-nonliteral -isystem %S/Inputs -fno-signed-char %s
// RUN: %clang_cc1 -fblocks -fsyntax-only -verify -Wformat-nonliteral -isystem %S/Inputs -triple=x86_64-unknown-fuchsia %s
// RUN: %clang_cc1 -fblocks -fsyntax-only -verify -Wformat-nonliteral -isystem %S/Inputs -triple=x86_64-linux-android %s

#include <stdarg.h>
#include <stddef.h>
#define __need_wint_t
#include <stddef.h> // For wint_t and wchar_t

typedef struct _FILE FILE;
int fprintf(FILE *, const char *restrict, ...);
int printf(const char *restrict, ...); // expected-note{{passing argument to parameter here}}
int snprintf(char *restrict, size_t, const char *restrict, ...);
int sprintf(char *restrict, const char *restrict, ...);
int vasprintf(char **, const char *, va_list);
int asprintf(char **, const char *, ...);
int vfprintf(FILE *, const char *restrict, va_list);
int vprintf(const char *restrict, va_list);
int vsnprintf(char *, size_t, const char *, va_list);
int vsprintf(char *restrict, const char *restrict, va_list); // expected-note{{passing argument to parameter here}}

int vscanf(const char *restrict format, va_list arg);

char * global_fmt;

void check_string_literal( FILE* fp, const char* s, char *buf, ... ) {

  char * b;
  va_list ap;
  va_start(ap,buf);

  printf(s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vprintf(s,ap); // expected-warning {{format string is not a string literal}}
  fprintf(fp,s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vfprintf(fp,s,ap); // expected-warning {{format string is not a string literal}}
  asprintf(&b,s); // expected-warning {{format string is not a string lit}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vasprintf(&b,s,ap); // expected-warning {{format string is not a string literal}}
  sprintf(buf,s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  snprintf(buf,2,s); // expected-warning {{format string is not a string lit}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  __builtin___sprintf_chk(buf,0,-1,s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  __builtin___snprintf_chk(buf,2,0,-1,s); // expected-warning {{format string is not a string lit}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vsprintf(buf,s,ap); // expected-warning {{format string is not a string lit}}
  vsnprintf(buf,2,s,ap); // expected-warning {{format string is not a string lit}}
  vsnprintf(buf,2,global_fmt,ap); // expected-warning {{format string is not a string literal}}
  __builtin___vsnprintf_chk(buf,2,0,-1,s,ap); // expected-warning {{format string is not a string lit}}
  __builtin___vsnprintf_chk(buf,2,0,-1,global_fmt,ap); // expected-warning {{format string is not a string literal}}

  vscanf(s, ap); // expected-warning {{format string is not a string literal}}

  const char *const fmt = "%d"; // FIXME -- defined here
  printf(fmt, 1, 2); // expected-warning{{data argument not used}}

  printf("abc"
         "%*d", 1, 1); // no-warning
  printf("abc\
def"
         "%*d", 1, 1); // no-warning

  // Allow 'unsigned' (instead of 'int') to be used for both the field width
  // and precision. This deviates from C99, but is reasonably safe and is also
  // accepted by GCC.
  printf("%*d", (unsigned) 1, 1); // no-warning
}

// When calling a non-variadic format function (vprintf, vscanf, NSLogv, ...),
// warn only if the format string argument is a parameter that is not itself
// declared as a format string with compatible format.
__attribute__((__format__ (__printf__, 2, 4)))
void check_string_literal2( FILE* fp, const char* s, char *buf, ... ) {
  char * b;
  va_list ap;
  va_start(ap,buf);

  printf(s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vprintf(s,ap); // no-warning
  fprintf(fp,s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vfprintf(fp,s,ap); // no-warning
  asprintf(&b,s); // expected-warning {{format string is not a string lit}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  vasprintf(&b,s,ap); // no-warning
  sprintf(buf,s); // expected-warning {{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  snprintf(buf,2,s); // expected-warning {{format string is not a string lit}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  __builtin___vsnprintf_chk(buf,2,0,-1,s,ap); // no-warning

  vscanf(s, ap); // expected-warning {{passing 'printf' format string where 'scanf' format string is expected}}
}

void check_conditional_literal(const char* s, int i) {
  printf(i == 1 ? "yes" : "no"); // no-warning
  printf(i == 0 ? (i == 1 ? "yes" : "no") : "dont know"); // no-warning
  printf(i == 0 ? (i == 1 ? s : "no") : "dont know"); // expected-warning{{format string is not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  printf("yes" ?: "no %d", 1); // expected-warning{{data argument not used by format string}}
  printf(0 ? "yes %s" : "no %d", 1); // no-warning
  printf(0 ? "yes %d" : "no %s", 1); // expected-warning{{format specifies type 'char *'}}

  printf(0 ? "yes" : "no %d", 1); // no-warning
  printf(0 ? "yes %d" : "no", 1); // expected-warning{{data argument not used by format string}}
  printf(1 ? "yes" : "no %d", 1); // expected-warning{{data argument not used by format string}}
  printf(1 ? "yes %d" : "no", 1); // no-warning
  printf(i ? "yes" : "no %d", 1); // no-warning
  printf(i ? "yes %s" : "no %d", 1); // expected-warning{{format specifies type 'char *'}}
  printf(i ? "yes" : "no %d", 1, 2); // expected-warning{{data argument not used by format string}}

  printf(i ? "%*s" : "-", i, s); // no-warning
  printf(i ? "yes" : 0 ? "no %*d" : "dont know %d", 1, 2); // expected-warning{{data argument not used by format string}}
  printf(i ? "%i\n" : "%i %s %s\n", i, s); // expected-warning{{more '%' conversions than data arguments}}
}

#if !defined(__ANDROID__) && !defined(__Fuchsia__)

void check_writeback_specifier(void)
{
  int x;
  char *b;
  printf("%n", b); // expected-warning{{format specifies type 'int *' but the argument has type 'char *'}}
  printf("%n", &x); // no-warning

  printf("%hhn", (signed char*)0); // no-warning
  printf("%hhn", (char*)0); // no-warning
  printf("%hhn", (unsigned char*)0); // no-warning
  printf("%hhn", (int*)0); // expected-warning{{format specifies type 'signed char *' but the argument has type 'int *'}}

  printf("%hn", (short*)0); // no-warning
  printf("%hn", (unsigned short*)0); // no-warning
  printf("%hn", (int*)0); // expected-warning{{format specifies type 'short *' but the argument has type 'int *'}}

  printf("%n", (int*)0); // no-warning
  printf("%n", (unsigned int*)0); // no-warning
  printf("%n", (char*)0); // expected-warning{{format specifies type 'int *' but the argument has type 'char *'}}

  printf("%ln", (long*)0); // no-warning
  printf("%ln", (unsigned long*)0); // no-warning
  printf("%ln", (int*)0); // expected-warning{{format specifies type 'long *' but the argument has type 'int *'}}

  printf("%lln", (long long*)0); // no-warning
  printf("%lln", (unsigned long long*)0); // no-warning
  printf("%lln", (int*)0); // expected-warning{{format specifies type 'long long *' but the argument has type 'int *'}}

  printf("%qn", (long long*)0); // no-warning
  printf("%qn", (unsigned long long*)0); // no-warning
  printf("%qn", (int*)0); // expected-warning{{format specifies type 'long long *' but the argument has type 'int *'}}

  printf("%Ln", 0); // expected-warning{{length modifier 'L' results in undefined behavior or no effect with 'n' conversion specifier}}
  // expected-note@-1{{did you mean to use 'll'?}}
}

#else

void check_writeback_specifier(void)
{
  int x;
  printf("%n", &x); // expected-warning{{'%n' specifier not supported on this platform}}

  printf("%hhn", (signed char*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%hhn", (char*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%hhn", (unsigned char*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%hhn", (int*)0); // expected-warning{{format specifies type 'signed char *' but the argument has type 'int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%hn", (short*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%hn", (unsigned short*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%hn", (int*)0); // expected-warning{{format specifies type 'short *' but the argument has type 'int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%n", (int*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%n", (unsigned int*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%n", (char*)0); // expected-warning{{format specifies type 'int *' but the argument has type 'char *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%ln", (long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%ln", (unsigned long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%ln", (int*)0); // expected-warning{{format specifies type 'long *' but the argument has type 'int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%lln", (long long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%lln", (unsigned long long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%lln", (int*)0); // expected-warning{{format specifies type 'long long *' but the argument has type 'int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%qn", (long long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%qn", (unsigned long long*)0); // expected-warning{{'%n' specifier not supported on this platform}}
}

#endif // !defined(__ANDROID__) && !defined(__Fuchsia__)

void check_invalid_specifier(FILE* fp, char *buf)
{
  printf("%s%lv%d","unix",10,20); // expected-warning {{invalid conversion specifier 'v'}} expected-warning {{data argument not used by format string}}
  fprintf(fp,"%%%l"); // expected-warning {{incomplete format specifier}}
  sprintf(buf,"%%%%%ld%d%d", 1, 2, 3); // expected-warning{{format specifies type 'long' but the argument has type 'int'}}
  snprintf(buf, 2, "%%%%%ld%;%d", 1, 2, 3); // expected-warning{{format specifies type 'long' but the argument has type 'int'}} expected-warning {{invalid conversion specifier ';'}} expected-warning {{data argument not used by format string}} \
                                            // expected-warning{{'snprintf' will always be truncated; specified size is 2, but format string expands to at least 7}}
}

void check_null_char_string(char* b)
{
  printf("\0this is bogus%d",1); // expected-warning {{string contains '\0'}}
  snprintf(b,10,"%%%%%d\0%d",1,2); // expected-warning {{string contains '\0'}}
  printf("%\0d",1); // expected-warning {{string contains '\0'}}
}

void check_empty_format_string(char* buf, ...)
{
  va_list ap;
  va_start(ap,buf);
  vprintf("",ap); // expected-warning {{format string is empty}}
  sprintf(buf, "", 1); // expected-warning {{format string is empty}}

  // Don't warn about empty format strings when there are no data arguments.
  // This can arise from macro expansions and non-standard format string
  // functions.
  sprintf(buf, ""); // no-warning
}

void check_wide_string(char* b, ...)
{
  va_list ap;
  va_start(ap,b);

  printf(L"foo %d",2); // expected-warning {{incompatible pointer types}}, expected-warning {{should not be a wide string}}
  vsprintf(b,L"bar %d",ap); // expected-warning {{incompatible pointer types}}, expected-warning {{should not be a wide string}}
}

void check_asterisk_precision_width(int x) {
  printf("%*d"); // expected-warning {{'*' specified field width is missing a matching 'int' argument}}
  printf("%.*d"); // expected-warning {{'.*' specified field precision is missing a matching 'int' argument}}
  printf("%*d",12,x); // no-warning
  printf("%*d","foo",x); // expected-warning {{field width should have type 'int', but argument has type 'char *'}}
  printf("%.*d","foo",x); // expected-warning {{field precision should have type 'int', but argument has type 'char *'}}
}

void __attribute__((format(printf,1,3))) myprintf(const char*, int blah, ...);

void test_myprintf(void) {
  myprintf("%d", 17, 18); // okay
}

void test_constant_bindings(void) {
  const char * const s1 = "hello";
  const char s2[] = "hello";
  const char *s3 = "hello";
  char * const s4 = "hello";
  extern const char s5[];

  printf(s1); // no-warning
  printf(s2); // no-warning
  printf(s3); // expected-warning{{not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  printf(s4); // expected-warning{{not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  printf(s5); // expected-warning{{not a string literal}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
}


// Test what happens when -Wformat-security only.
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic warning "-Wformat-security"

void test9(char *P) {
  int x;
  printf(P);   // expected-warning {{format string is not a string literal (potentially insecure)}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
  printf(P, 42);
}

void torture(va_list v8) {
  vprintf ("%*.*d", v8);  // no-warning

}

void test10(int x, float f, int i, long long lli) {
  printf("%s"); // expected-warning{{more '%' conversions than data arguments}}
  printf("%@", 12); // expected-warning{{invalid conversion specifier '@'}}
  printf("\0"); // expected-warning{{format string contains '\0' within the string body}}
  printf("xs\0"); // expected-warning{{format string contains '\0' within the string body}}
  printf("%*d\n"); // expected-warning{{'*' specified field width is missing a matching 'int' argument}}
  printf("%*.*d\n", x); // expected-warning{{'.*' specified field precision is missing a matching 'int' argument}}
  printf("%*d\n", f, x); // expected-warning{{field width should have type 'int', but argument has type 'double'}}
  printf("%*.*d\n", x, f, x); // expected-warning{{field precision should have type 'int', but argument has type 'double'}}
  printf("%**\n"); // expected-warning{{invalid conversion specifier '*'}}
  printf("%d%d\n", x); // expected-warning{{more '%' conversions than data arguments}}
  printf("%d\n", x, x); // expected-warning{{data argument not used by format string}}
  printf("%W%d\n", x, x); // expected-warning{{invalid conversion specifier 'W'}}  expected-warning {{data argument not used by format string}}
  printf("%"); // expected-warning{{incomplete format specifier}}
  printf("%.d", x); // no-warning
  printf("%.", x);  // expected-warning{{incomplete format specifier}}
  printf("%f", 4); // expected-warning{{format specifies type 'double' but the argument has type 'int'}}
  printf("%qd", lli); // no-warning
  printf("%qd", x); // expected-warning{{format specifies type 'long long' but the argument has type 'int'}}
  printf("%qp", (void *)0); // expected-warning{{length modifier 'q' results in undefined behavior or no effect with 'p' conversion specifier}}
  printf("hhX %hhX", (unsigned char)10); // no-warning
  printf("llX %llX", (long long) 10); // no-warning
  printf("%lb %lB", (long) 10, (long) 10); // no-warning
  printf("%llb %llB", (long long) 10, (long long) 10); // no-warning
  // This is fine, because there is an implicit conversion to an int.
  printf("%d", (unsigned char) 10); // no-warning
  printf("%d", (long long) 10); // expected-warning{{format specifies type 'int' but the argument has type 'long long'}}
  printf("%Lf\n", (long double) 1.0); // no-warning
  printf("%f\n", (long double) 1.0); // expected-warning{{format specifies type 'double' but the argument has type 'long double'}}
  // The man page says that a zero precision is okay.
  printf("%.0Lf", (long double) 1.0); // no-warning
  printf("%c\n", "x"); // expected-warning{{format specifies type 'int' but the argument has type 'char *'}}
  printf("%c\n", 1.23); // expected-warning{{format specifies type 'int' but the argument has type 'double'}}
  printf("Format %d, is %! %f", 1, 4.4); // expected-warning{{invalid conversion specifier '!'}}
}

typedef unsigned char uint8_t;

void should_understand_small_integers(void) {
  printf("%hhu", (short) 10); // expected-warning{{format specifies type 'unsigned char' but the argument has type 'short'}}
  printf("%hu\n", (unsigned char)1); // warning with -Wformat-pedantic only
  printf("%hu\n", (uint8_t)1);       // warning with -Wformat-pedantic only
}

void test11(void *p, char *s) {
  printf("%p", p); // no-warning
  printf("%p", 123); // expected-warning{{format specifies type 'void *' but the argument has type 'int'}}
  printf("%.4p", p); // expected-warning{{precision used with 'p' conversion specifier, resulting in undefined behavior}}
  printf("%+p", p); // expected-warning{{flag '+' results in undefined behavior with 'p' conversion specifier}}
  printf("% p", p); // expected-warning{{flag ' ' results in undefined behavior with 'p' conversion specifier}}
  printf("%0p", p); // expected-warning{{flag '0' results in undefined behavior with 'p' conversion specifier}}
  printf("%s", s); // no-warning
  printf("%+s", p); // expected-warning{{flag '+' results in undefined behavior with 's' conversion specifier}}
                    // expected-warning@-1 {{format specifies type 'char *' but the argument has type 'void *'}}
  printf("% s", p); // expected-warning{{flag ' ' results in undefined behavior with 's' conversion specifier}}
                    // expected-warning@-1 {{format specifies type 'char *' but the argument has type 'void *'}}
  printf("%0s", p); // expected-warning{{flag '0' results in undefined behavior with 's' conversion specifier}}
                    // expected-warning@-1 {{format specifies type 'char *' but the argument has type 'void *'}}
}

void test12(char *b) {
  unsigned char buf[4];
  printf ("%.4s\n", buf); // no-warning
  printf ("%.4s\n", &buf); // expected-warning{{format specifies type 'char *' but the argument has type 'unsigned char (*)[4]'}}

  // Verify that we are checking asprintf
  asprintf(&b, "%d", "asprintf"); // expected-warning{{format specifies type 'int' but the argument has type 'char *'}}
}

void test13(short x) {
  char bel = 007;
  printf("bel: '0%hhd'\n", bel); // no-warning
  printf("x: '0%hhd'\n", x); // expected-warning {{format specifies type 'char' but the argument has type 'short'}}
}

typedef struct __aslclient *aslclient;
typedef struct __aslmsg *aslmsg;
int asl_log(aslclient asl, aslmsg msg, int level, const char *format, ...) __attribute__((__format__ (__printf__, 4, 5)));
void test_asl(aslclient asl) {
  asl_log(asl, 0, 3, "Error: %m"); // no-warning
  asl_log(asl, 0, 3, "Error: %W"); // expected-warning{{invalid conversion specifier 'W'}}
}

typedef enum { A } int_t;
void f0(int_t x) { printf("%d\n", x); }

// Unicode test cases.  These are possibly specific to Mac OS X.  If so, they should
// eventually be moved into a separate test.

void test_unicode_conversions(wchar_t *s) {
  printf("%S", s); // no-warning
  printf("%s", s); // expected-warning{{format specifies type 'char *' but the argument has type 'wchar_t *'}}
  printf("%C", s[0]); // no-warning
#if defined(__sun) && !defined(__LP64__)
  printf("%c", s[0]); // expected-warning{{format specifies type 'int' but the argument has type 'wchar_t' (aka 'long')}}
#else
  printf("%c", s[0]);
#endif
  // FIXME: This test reports inconsistent results. On Windows, '%C' expects
  // 'unsigned short'.
  // printf("%C", 10);
  printf("%S", "hello"); // expected-warning{{but the argument has type 'char *'}}
}

// Mac OS X supports positional arguments in format strings.
// This is an IEEE extension (IEEE Std 1003.1).
// FIXME: This is probably not portable everywhere.
void test_positional_arguments(void) {
  printf("%0$", (int)2); // expected-warning{{position arguments in format strings start counting at 1 (not 0)}}
  printf("%1$*0$d", (int) 2); // expected-warning{{position arguments in format strings start counting at 1 (not 0)}}
  printf("%1$d", (int) 2); // no-warning
  printf("%1$d", (int) 2, 2); // expected-warning{{data argument not used by format string}}
  printf("%1$d%1$f", (int) 2); // expected-warning{{format specifies type 'double' but the argument has type 'int'}}
  printf("%1$2.2d", (int) 2); // no-warning
  printf("%2$*1$.2d", (int) 2, (int) 3); // no-warning
  printf("%2$*8$d", (int) 2, (int) 3); // expected-warning{{specified field width is missing a matching 'int' argument}}
  printf("%%%1$d", (int) 2); // no-warning
  printf("%1$d%%", (int) 2); // no-warning
}

// PR 6697 - Handle format strings where the data argument is not adjacent to the format string
void myprintf_PR_6697(const char *format, int x, ...) __attribute__((__format__(printf,1, 3)));
void test_pr_6697(void) {
  myprintf_PR_6697("%s\n", 1, "foo"); // no-warning
  myprintf_PR_6697("%s\n", 1, (int)0); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
  // FIXME: Not everything should clearly support positional arguments,
  // but we need a way to identify those cases.
  myprintf_PR_6697("%1$s\n", 1, "foo"); // no-warning
  myprintf_PR_6697("%2$s\n", 1, "foo"); // expected-warning{{data argument position '2' exceeds the number of data arguments (1)}}
  myprintf_PR_6697("%18$s\n", 1, "foo"); // expected-warning{{data argument position '18' exceeds the number of data arguments (1)}}
  myprintf_PR_6697("%1$s\n", 1, (int) 0); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
}

void rdar8026030(FILE *fp) {
  fprintf(fp, "\%"); // expected-warning{{incomplete format specifier}}
}

void bug7377_bad_length_mod_usage(void) {
  // Bad length modifiers
  printf("%hhs", "foo"); // expected-warning{{length modifier 'hh' results in undefined behavior or no effect with 's' conversion specifier}}
  printf("%1$zp", (void *)0); // expected-warning{{length modifier 'z' results in undefined behavior or no effect with 'p' conversion specifier}}
  printf("%ls", L"foo"); // no-warning
  printf("%#.2Lf", (long double)1.234); // no-warning

  // Bad flag usage
  printf("%#p", (void *) 0); // expected-warning{{flag '#' results in undefined behavior with 'p' conversion specifier}}
  printf("%0d", -1); // no-warning
  printf("%0b%0B", -1u, -1u); // no-warning
  printf("%-p", (void *) 0); // no-warning
#if !defined(__ANDROID__) && !defined(__Fuchsia__)
  printf("%#n", (int *) 0); // expected-warning{{flag '#' results in undefined behavior with 'n' conversion specifier}}
  printf("%-n", (int *) 0); // expected-warning{{flag '-' results in undefined behavior with 'n' conversion specifier}}
#else
  printf("%#n", (int *) 0); // expected-warning{{flag '#' results in undefined behavior with 'n' conversion specifier}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
  printf("%-n", (int *) 0); // expected-warning{{flag '-' results in undefined behavior with 'n' conversion specifier}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
#endif // !defined(__ANDROID__) && !defined(__Fuchsia__)

  // Bad optional amount use
  printf("%.2c", 'a'); // expected-warning{{precision used with 'c' conversion specifier, resulting in undefined behavior}}
#if !defined(__ANDROID__) && !defined(__Fuchsia__)
  printf("%1n", (int *) 0); // expected-warning{{field width used with 'n' conversion specifier, resulting in undefined behavior}}
  printf("%.9n", (int *) 0); // expected-warning{{precision used with 'n' conversion specifier, resulting in undefined behavior}}
#else
  printf("%1n", (int *) 0); // expected-warning{{field width used with 'n' conversion specifier, resulting in undefined behavior}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
  printf("%.9n", (int *) 0); // expected-warning{{precision used with 'n' conversion specifier, resulting in undefined behavior}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
#endif // #if !defined(__ANDROID__) && !defined(__Fuchsia__)

  // Ignored flags
  printf("% +f", 1.23); // expected-warning{{flag ' ' is ignored when flag '+' is present}}
  printf("%+ f", 1.23); // expected-warning{{flag ' ' is ignored when flag '+' is present}}
  printf("%0-f", 1.23); // expected-warning{{flag '0' is ignored when flag '-' is present}}
  printf("%-0f", 1.23); // expected-warning{{flag '0' is ignored when flag '-' is present}}
  printf("%-+f", 1.23); // no-warning
}

// PR 7981 - handle '%lc' (wint_t)

void pr7981(wint_t c, wchar_t c2) {
  printf("%lc", c); // no-warning
  printf("%lc", 1.0); // expected-warning{{the argument has type 'double'}}
#if __WINT_WIDTH__ == 32 && !(defined(__sun) && !defined(__LP64__))
  printf("%lc", (char) 1); // no-warning
#else
  printf("%lc", (char) 1); // expected-warning{{the argument has type 'char'}}
#endif
  printf("%lc", &c); // expected-warning{{the argument has type 'wint_t *'}}
  // If wint_t and wchar_t are the same width and wint_t is signed where
  // wchar_t is unsigned, an implicit conversion isn't possible.
#if defined(__WINT_UNSIGNED__) || !defined(__WCHAR_UNSIGNED__) ||   \
  __WINT_WIDTH__ > __WCHAR_WIDTH__
  printf("%lc", c2); // no-warning
#endif
}

// -Wformat-security says NULL is not a string literal
void rdar8269537(void) {
  // This is likely to crash in most cases, but -Wformat-nonliteral technically
  // doesn't warn in this case.
  printf(0); // no-warning
}

// Handle functions with multiple format attributes.
extern void rdar8332221_vprintf_scanf(const char *, va_list, const char *, ...)
     __attribute__((__format__(__printf__, 1, 0)))
     __attribute__((__format__(__scanf__, 3, 4)));

void rdar8332221(va_list ap, int *x, long *y) {
  rdar8332221_vprintf_scanf("%", ap, "%d", x); // expected-warning{{incomplete format specifier}}
}

void rdar8332221_vprintf_scanf(const char *p, va_list ap, const char *s, ...) {
  vprintf(p, ap);

  va_list vs;
  va_start(vs, s);
  vscanf(s, vs);
  va_end(vs);
}

__attribute__((__format__(__printf__, 1, 0)))
__attribute__((__format__(__scanf__, 3, 4)))
void vprintf_scanf_bad(const char *p, va_list ap, const char *s, ...) {
  vscanf(p, ap); // expected-warning{{passing 'printf' format string where 'scanf' format string is expected}}

  va_list vs;
  va_start(vs, s);
  vprintf(s, vs); // expected-warning{{passing 'scanf' format string where 'printf' format string is expected}}
  va_end(vs);
}

// PR8641
void pr8641(void) {
  printf("%#x\n", 10);
  printf("%#X\n", 10);
  printf("%#b %#15.8B\n", 10, 10u);
}

void posix_extensions(void) {
  // Test %'d, "thousands grouping".
  printf("%'d\n", 123456789); // no-warning
  printf("%'i\n", 123456789); // no-warning
  printf("%'f\n", (float) 1.0); // no-warning
  printf("%'p\n", (void*) 0); // expected-warning{{results in undefined behavior with 'p' conversion specifier}}
  printf("%'b\n", 123456789); // expected-warning{{results in undefined behavior with 'b' conversion specifier}}
  printf("%'B\n", 123456789); // expected-warning{{results in undefined behavior with 'B' conversion specifier}}
}

// PR8486
//
// Test what happens when -Wformat is on, but -Wformat-security is off.
#pragma GCC diagnostic warning "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-security"

void pr8486(void) {
  printf("%s", 1); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
}

// PR9314
// Don't warn about string literals that are PreDefinedExprs, e.g. __func__.
void pr9314(void) {
  printf(__PRETTY_FUNCTION__); // no-warning
  printf(__func__); // no-warning
}

int printf(const char * restrict, ...) __attribute__((__format__ (__printf__, 1, 2)));

void rdar9612060(void) {
  printf("%s", 2); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
}

void check_char(unsigned char x, signed char y) {
  printf("%c", y); // no-warning
  printf("%hhu", x); // no-warning
  printf("%hhi", y); // no-warning
  printf("%hhi", x); // no-warning
  printf("%c", x); // no-warning
  printf("%hhu", y); // no-warning
  printf("%hhb %hhB", x, x); // no-warning
}

// Test suppression of individual warnings.

void test_suppress_invalid_specifier(void) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-invalid-specifier"
  printf("%@", 12); // no-warning
#pragma clang diagnostic pop
}

// Make sure warnings are on for next test.
#pragma GCC diagnostic warning "-Wformat"
#pragma GCC diagnostic warning "-Wformat-security"

// Test that the printf call site is where the warning is attached.  If the
// format string is somewhere else, point to it in a note.
void pr9751(void) {
  const char kFormat1[] = "%d %d \n"; // expected-note{{format string is defined here}}}
  printf(kFormat1, 0); // expected-warning{{more '%' conversions than data arguments}}
  printf("%d %s\n", 0); // expected-warning{{more '%' conversions than data arguments}}

  const char kFormat2[] = "%18$s\n"; // expected-note{{format string is defined here}}
  printf(kFormat2, 1, "foo"); // expected-warning{{data argument position '18' exceeds the number of data arguments (2)}}
  printf("%18$s\n", 1, "foo"); // expected-warning{{data argument position '18' exceeds the number of data arguments (2)}}

  const char kFormat4[] = "%y"; // expected-note{{format string is defined here}}
  printf(kFormat4, 5); // expected-warning{{invalid conversion specifier 'y'}}
  printf("%y", 5); // expected-warning{{invalid conversion specifier 'y'}}

  const char kFormat5[] = "%."; // expected-note{{format string is defined here}}
  printf(kFormat5, 5); // expected-warning{{incomplete format specifier}}
  printf("%.", 5); // expected-warning{{incomplete format specifier}}

  const char kFormat6[] = "%s"; // expected-note{{format string is defined here}}
  printf(kFormat6, 5); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
  printf("%s", 5); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}

  const char kFormat7[] = "%0$"; // expected-note{{format string is defined here}}
  printf(kFormat7, 5); // expected-warning{{position arguments in format strings start counting at 1 (not 0)}}
  printf("%0$", 5); // expected-warning{{position arguments in format strings start counting at 1 (not 0)}}

  const char kFormat8[] = "%1$d %d"; // expected-note{{format string is defined here}}
  printf(kFormat8, 4, 4); // expected-warning{{cannot mix positional and non-positional arguments in format string}}
  printf("%1$d %d", 4, 4); // expected-warning{{cannot mix positional and non-positional arguments in format string}}

  const char kFormat9[] = ""; // expected-note{{format string is defined here}}
  printf(kFormat9, 4, 4); // expected-warning{{format string is empty}}
  printf("", 4, 4); // expected-warning{{format string is empty}}

  const char kFormat10[] = "\0%d"; // expected-note{{format string is defined here}}
  printf(kFormat10, 4); // expected-warning{{format string contains '\0' within the string body}}
  printf("\0%d", 4); // expected-warning{{format string contains '\0' within the string body}}

  const char kFormat11[] = "%*d"; // expected-note{{format string is defined here}}
  printf(kFormat11); // expected-warning{{'*' specified field width is missing a matching 'int' argument}}
  printf("%*d"); // expected-warning{{'*' specified field width is missing a matching 'int' argument}}

  const char kFormat12[] = "%*d"; // expected-note{{format string is defined here}}
  printf(kFormat12, 4.4); // expected-warning{{field width should have type 'int', but argument has type 'double'}}
  printf("%*d", 4.4); // expected-warning{{field width should have type 'int', but argument has type 'double'}}

  const char kFormat13[] = "%.3p"; // expected-note{{format string is defined here}}
  void *p;
  printf(kFormat13, p); // expected-warning{{precision used with 'p' conversion specifier, resulting in undefined behavior}}
  printf("%.3p", p); // expected-warning{{precision used with 'p' conversion specifier, resulting in undefined behavior}}

  const char kFormat14[] = "%0s"; // expected-note{{format string is defined here}}
  printf(kFormat14, "a"); // expected-warning{{flag '0' results in undefined behavior with 's' conversion specifier}}
  printf("%0s", "a"); // expected-warning{{flag '0' results in undefined behavior with 's' conversion specifier}}

  const char kFormat15[] = "%hhs"; // expected-note{{format string is defined here}}
  printf(kFormat15, "a"); // expected-warning{{length modifier 'hh' results in undefined behavior or no effect with 's' conversion specifier}}
  printf("%hhs", "a"); // expected-warning{{length modifier 'hh' results in undefined behavior or no effect with 's' conversion specifier}}

  const char kFormat16[] = "%-0d"; // expected-note{{format string is defined here}}
  printf(kFormat16, 5); // expected-warning{{flag '0' is ignored when flag '-' is present}}
  printf("%-0d", 5); // expected-warning{{flag '0' is ignored when flag '-' is present}}

  // Make sure that the "format string is defined here" note is not emitted
  // when the original string is within the argument expression.
  printf(1 ? "yes %d" : "no %d"); // expected-warning{{more '%' conversions than data arguments}}

  const char kFormat17[] = "%hu"; // expected-note{{format string is defined here}}}
  printf(kFormat17, (int[]){0}); // expected-warning{{format specifies type 'unsigned short' but the argument}}

  printf("%a", (long double)0); // expected-warning{{format specifies type 'double' but the argument has type 'long double'}}

  // Test braced char[] initializers.
  const char kFormat18[] = { "%lld" }; // expected-note{{format string is defined here}}
  printf(kFormat18, 0); // expected-warning{{format specifies type}}

  // Make sure we point at the offending argument rather than the format string.
  const char kFormat19[] = "%d";  // expected-note{{format string is defined here}}
  printf(kFormat19,
         0.0); // expected-warning{{format specifies}}
}

void pr18905(void) {
  const char s1[] = "s\0%s"; // expected-note{{format string is defined here}}
  const char s2[1] = "s"; // expected-note{{format string is defined here}}
  const char s3[2] = "s\0%s"; // expected-warning{{initializer-string for char array is too long}}
  const char s4[10] = "s";
  const char s5[0] = "%s"; // expected-warning{{initializer-string for char array is too long}}
                           // expected-note@-1{{format string is defined here}}

  printf(s1); // expected-warning{{format string contains '\0' within the string body}}
  printf(s2); // expected-warning{{format string is not null-terminated}}
  printf(s3); // no-warning
  printf(s4); // no-warning
  printf(s5); // expected-warning{{format string is not null-terminated}}
}

void __attribute__((format(strfmon,1,2))) monformat(const char *fmt, ...);
void __attribute__((format(strftime,1,0))) dateformat(const char *fmt);

// Other formats
void test_other_formats(void) {
  char *str = "";
  monformat("", 1); // expected-warning{{format string is empty}}
  monformat(str); // expected-warning{{format string is not a string literal (potentially insecure)}}
  dateformat(""); // expected-warning{{format string is empty}}
  dateformat(str); // no-warning (using strftime non-literal is not unsafe)
}

// Do not warn about unused arguments coming from system headers.
#include <format-unused-system-args.h>
void test_unused_system_args(int x) {
  PRINT1("%d\n", x); // no-warning{{extra argument is system header is OK}}
}

void pr12761(char c) {
  // This should not warn even with -fno-signed-char.
  printf("%hhx", c);
}

void test_opencl_vector_format(int x) {
  printf("%v4d", x); // expected-warning{{invalid conversion specifier 'v'}}
  printf("%vd", x); // expected-warning{{invalid conversion specifier 'v'}}
  printf("%0vd", x); // expected-warning{{invalid conversion specifier 'v'}}
  printf("%hlf", x); // expected-warning{{invalid conversion specifier 'l'}}
  printf("%hld", x); // expected-warning{{invalid conversion specifier 'l'}}
}

// Test that we correctly merge the format in both orders.
extern void test14_foo(const char *, const char *, ...)
     __attribute__((__format__(__printf__, 1, 3)));
extern void test14_foo(const char *, const char *, ...)
     __attribute__((__format__(__scanf__, 2, 3)));

extern void test14_bar(const char *, const char *, ...)
     __attribute__((__format__(__scanf__, 2, 3)));
extern void test14_bar(const char *, const char *, ...)
     __attribute__((__format__(__printf__, 1, 3)));

void test14_zed(int *p) {
  test14_foo("%", "%d", p); // expected-warning{{incomplete format specifier}}
  test14_bar("%", "%d", p); // expected-warning{{incomplete format specifier}}
}

#if !defined(__ANDROID__) && !defined(__Fuchsia__)

void test_qualifiers(volatile int *vip, const int *cip,
                     const volatile int *cvip) {
  printf("%n", cip); // expected-warning{{format specifies type 'int *' but the argument has type 'const int *'}}
  printf("%n", cvip); // expected-warning{{format specifies type 'int *' but the argument has type 'const volatile int *'}}

  printf("%n", vip); // No warning.
  printf("%p", cip); // No warning.
  printf("%p", cvip); // No warning.


  typedef int* ip_t;
  typedef const int* cip_t;
  printf("%n", (ip_t)0); // No warning.
  printf("%n", (cip_t)0); // expected-warning{{format specifies type 'int *' but the argument has type 'cip_t' (aka 'const int *')}}
}

#else

void test_qualifiers(volatile int *vip, const int *cip,
                     const volatile int *cvip) {
  printf("%n", cip); // expected-warning{{format specifies type 'int *' but the argument has type 'const int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
  printf("%n", cvip); // expected-warning{{format specifies type 'int *' but the argument has type 'const volatile int *'}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}

  printf("%n", vip); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%p", cip); // No warning.
  printf("%p", cvip); // No warning.


  typedef int* ip_t;
  typedef const int* cip_t;
  printf("%n", (ip_t)0); // expected-warning{{'%n' specifier not supported on this platform}}
  printf("%n", (cip_t)0); // expected-warning{{format specifies type 'int *' but the argument has type 'cip_t' (aka 'const int *')}}
  // expected-warning@-1 {{'%n' specifier not supported on this platform}}
}

#endif // #if !defined(__ANDROID__) && !defined(__Fuchsia__)

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic warning "-Wformat-security"
extern void test_format_security_extra_args(const char*, int, ...)
    __attribute__((__format__(__printf__, 1, 3)));
void test_format_security_pos(char* string) {
  test_format_security_extra_args(string, 5); // expected-warning {{format string is not a string literal (potentially insecure)}}
  // expected-note@-1{{treat the string as an argument to avoid this}}
}
#pragma GCC diagnostic warning "-Wformat-nonliteral"

void test_char_pointer_arithmetic(int b) {
  const char s1[] = "string";
  const char s2[] = "%s string";

  printf(s1 - 1);  // expected-warning {{format string is not a string literal (potentially insecure)}}
  // expected-note@-1{{treat the string as an argument to avoid this}}

  printf(s1 + 2);  // no-warning
  printf(s2 + 2);  // no-warning

  const char s3[] = "%s string";
  printf((s3 + 2) - 2);  // expected-warning{{more '%' conversions than data arguments}}
  // expected-note@-2{{format string is defined here}}
  printf(2 + s2);             // no-warning
  printf(6 + s2 - 2);         // no-warning
  printf(2 + (b ? s1 : s2));  // no-warning

  const char s5[] = "string %s";
  printf(2 + (b ? s2 : s5));  // expected-warning{{more '%' conversions than data arguments}}
  // expected-note@-2{{format string is defined here}}
  printf(2 + (b ? s2 : s5), "");      // no-warning
  printf(2 + (b ? s1 : s2 - 2), "");  // no-warning

  const char s6[] = "%s string";
  printf(2 + (b ? s1 : s6 - 2));  // expected-warning{{more '%' conversions than data arguments}}
  // expected-note@-2{{format string is defined here}}
  printf(1 ? s2 + 2 : s2);  // no-warning
  printf(0 ? s2 : s2 + 2);  // no-warning
  printf(2 + s2 + 5 * 3 - 16, "");  // expected-warning{{data argument not used}}

  const char s7[] = "%s string %s %s";
  printf(s7 + 3, "");  // expected-warning{{more '%' conversions than data arguments}}
  // expected-note@-2{{format string is defined here}}
}

void PR30481(void) {
  // This caused crashes due to invalid casts.
  printf(1 > 0); // expected-warning{{format string is not a string literal}} expected-error{{incompatible integer to pointer conversion}} expected-note@format-strings.c:*{{passing argument to parameter here}} expected-note{{to avoid this}}
}

void test_printf_opaque_ptr(void *op) {
  printf("%s", op); // expected-warning{{format specifies type 'char *' but the argument has type 'void *'}}
}

void test_block(void) {
  void __attribute__((__format__(__printf__, 1, 2))) (^printf_arg1)(
      const char *, ...) =
      ^(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2))) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  };

  printf_arg1("%s string %i\n", "aaa", 123);
  printf_arg1("%s string\n", 123); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}

  void __attribute__((__format__(__printf__, 2, 3))) (^printf_arg2)(
      const char *, const char *, ...) =
      ^(const char *not_fmt, const char *fmt, ...)
          __attribute__((__format__(__printf__, 2, 3))) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    vprintf(not_fmt, ap); // expected-warning{{format string is not a string literal}}
    va_end(ap);
  };

  printf_arg2("foo", "%s string %i\n", "aaa", 123);
  printf_arg2("%s string\n", "foo", "bar"); // expected-warning{{data argument not used by format string}}
}

void test_promotion(void) {
  // Default argument promotions for *printf in N2562
  // https://github.com/llvm/llvm-project/issues/57102
  // N2562: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2562.pdf
  int i;
  signed char sc;
  unsigned char uc;
  char c;
  short ss;
  unsigned short us;

  printf("%hhd %hd %d %hhd %hd %d", i, i, i, sc, sc, sc); // no-warning
  printf("%hhd %hd %d %hhd %hd %d", uc, uc, uc, c, c, c); // no-warning

  // %ld %lld %llx
  printf("%ld", i); // expected-warning{{format specifies type 'long' but the argument has type 'int'}}
  printf("%lld", i); // expected-warning{{format specifies type 'long long' but the argument has type 'int'}}
  printf("%ld", sc); // expected-warning{{format specifies type 'long' but the argument has type 'signed char'}}
  printf("%lld", sc); // expected-warning{{format specifies type 'long long' but the argument has type 'signed char'}}
  printf("%ld", uc); // expected-warning{{format specifies type 'long' but the argument has type 'unsigned char'}}
  printf("%lld", uc); // expected-warning{{format specifies type 'long long' but the argument has type 'unsigned char'}}
  printf("%llx", i); // expected-warning{{format specifies type 'unsigned long long' but the argument has type 'int'}}

  // ill formed spec for floats
  printf("%hf", // expected-warning{{length modifier 'h' results in undefined behavior or no effect with 'f' conversion specifier}}
  sc); // expected-warning{{format specifies type 'double' but the argument has type 'signed char'}}

  // for %hhd and `short` they are compatible by promotions but more likely misuse
  printf("%hd", ss); // no-warning
  printf("%hhd", ss); // expected-warning{{format specifies type 'char' but the argument has type 'short'}}
  printf("%hu", us); // no-warning
  printf("%hhu", ss); // expected-warning{{format specifies type 'unsigned char' but the argument has type 'short'}}

  // floats & integers are not compatible
  printf("%f", i); // expected-warning{{format specifies type 'double' but the argument has type 'int'}}
  printf("%f", sc); // expected-warning{{format specifies type 'double' but the argument has type 'signed char'}}
  printf("%f", uc); // expected-warning{{format specifies type 'double' but the argument has type 'unsigned char'}}
  printf("%f", c); // expected-warning{{format specifies type 'double' but the argument has type 'char'}}
  printf("%f", ss); // expected-warning{{format specifies type 'double' but the argument has type 'short'}}
  printf("%f", us); // expected-warning{{format specifies type 'double' but the argument has type 'unsigned short'}}

  // character literals
  // In C language engineering practice, printing a character literal with %hhd or %d is common, but %hd may be misuse.
  printf("%hhu", 'a'); // no-warning
  printf("%hhd", 'a'); // no-warning
  printf("%hd", 'a'); // expected-warning{{format specifies type 'short' but the argument has type 'char'}}
  printf("%hu", 'a'); // expected-warning{{format specifies type 'unsigned short' but the argument has type 'char'}}
  printf("%d", 'a'); // no-warning
  printf("%u", 'a'); // no-warning
  
  // pointers
  printf("%s", i); // expected-warning{{format specifies type 'char *' but the argument has type 'int'}}
}
