/**
 * vector
 * CS 341 - Fall 2023
 */
#include "sstring.h"
#include <assert.h>

bool equals_string(char *str1, char *str2)
{

    while (*str1 && *str2)
    {
        if (*str1 != *str2)
            return 0;
        else
        {
            str1++;
            str2++;
        }
    }

    return (*str1 == '\0' && *str2 == '\0');
}
int main(int argc, char *argv[])
{
    // TODO create some tests

    // test1: sstr_to_string & cstr_to_sstring
    char *str = "Hi there!";
    sstring *sstr = cstr_to_sstring(str);
    printf("Expected: %s, got: %s\n", sstring_to_cstr(sstr), str);
    assert(equals_string(sstring_to_cstr(sstr), str));

    // test2: sstring_append
    //  * sstring *str1 = cstr_to_sstring("abc");
    //  * sstring *str2 = sstring_to_cstr("def");
    //  * int len = sstring_append(str1, str2); // len == 6
    //  * sstring_to_cstr(str1); // == "abcdef"
    sstring *str1 = cstr_to_sstring("abc");
    sstring *str2 = cstr_to_sstring("def");
    int len = sstring_append(str1, str2); // side-effects: Modifies `str1`
    char *obtained = sstring_to_cstr(str1);
    char *expected = "abcdef";
    printf("Expected: %s, got: %s\n", expected, obtained);
    assert(equals_string(expected, obtained));
    printf("Expected len of 6, obtained: len of %d\n", len);
    assert(len == 6);

    // test3.1 & 3.2: sstring_split 
    //  * Example:
    //  * sstring_split(cstr_to_sstring("abcdeefg"), 'e'); // == [ "abcd", "", "fg" ]);
    //  * sstring_split(cstr_to_sstring("This is a sentence."), ' ');
    //  * // == [ "This", "is", "a", "sentence." ]
    //  */
     
     // test 3.1
    vector *v1 = sstring_split(cstr_to_sstring("abcdeefg"), 'e');

    printf("Expected: abcd, got: %s\n", (char *)*vector_begin(v1));
    assert(equals_string((char *)*vector_begin(v1), "abcd"));
    printf("Expected:  , got: %s\n", (char *)*vector_at(v1, (size_t) 1));
    assert(equals_string((char *)*vector_at(v1, (size_t) 1), ""));
    printf("Expected: fg, got: %s\n", (char *)*vector_at(v1, (size_t) 2));
    assert(equals_string((char *)*vector_at(v1, (size_t) 2), "fg"));

    // test 3.2
    vector *v2 = sstring_split(cstr_to_sstring("This is a sentence."), ' ');

    printf("Expected: This, got: %s\n", (char *)*vector_begin(v2));
    assert(equals_string((char *)*vector_begin(v2), "This"));
    printf("Expected: is, got: %s\n", (char *)*vector_at(v2, (size_t) 1));
    assert(equals_string((char *)*vector_at(v2, (size_t) 1), "is"));
    printf("Expected: a, got: %s\n", (char *)*vector_at(v2, (size_t) 2));
    assert(equals_string((char *)*vector_at(v2, (size_t) 2), "a"));
    printf("Expected: sentence, got: %s\n", (char *)*vector_back(v2));
    assert(equals_string((char *)*vector_begin(v2), "This"));


    puts("All tests pass. Hooray!");
    return 0;
}
