// C program for illustration of regcomp()
#include <stdio.h>
#include <regex.h>
int match(const char *string, const char *pattern) {
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) return 0;
    int status = regexec(&re, string, 0, NULL, 0);
    regfree(&re);
    if (status != 0) return 0;
    return 1;
}
int main(void) {
    const char* s1 = "abc";
    const char* s2 = "123";
    const char* re = "[1-9]+";
    printf("%s Given string matches %s? %s\n", s1, re, match(s1, re) ? "true" : "false");
    printf("%s Given string matches %s? %s\n", s2, re, match(s2, re) ? "true" : "false");
}
