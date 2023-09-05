/**
 * vector
 * CS 341 - Fall 2023
 */
#include "sstring.h"
#include <assert.h>
#include <string.h>

bool equals_string(char *str1, char *str2)
{

    assert(str1);
    assert(str2);
    return strcmp(str1, str2) == 0;

    // while (*str1 && *str2)
    // {
    //     if (*str1 != *str2)
    //         return 0;
    //     else
    //     {
    //         str1++;
    //         str2++;
    //     }
    // }

    // return (*str1 == '\0' && *str2 == '\0');
}
int main(int argc, char *argv[])
{
    // TODO create some tests

    // test1: sstr_to_string & cstr_to_sstring
    char *str = "Hi there!";
    sstring *sstr = cstr_to_sstring(str);
    char *expected_str = sstring_to_cstr(sstr);
    printf("Expected: %s, got: %s\n", expected_str, str);
    assert(equals_string(expected_str, str));
    free(expected_str);
    sstring_destroy(sstr);

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
    free(obtained);

    printf("Expected len of 6, obtained: len of %d\n", len);
    assert(len == 6);
    sstring_destroy(str1);
    sstring_destroy(str2);

    // test3.1 & 3.2: sstring_split
    //  * Example:
    //  * sstring_split(cstr_to_sstring("abcdeefg"), 'e'); // == [ "abcd", "", "fg" ]);
    //  * sstring_split(cstr_to_sstring("This is a sentence."), ' ');
    //  * // == [ "This", "is", "a", "sentence." ]
    //  */

    // test 3.1
    str1 = cstr_to_sstring("abcdeefg");
    vector *v1 = sstring_split(str1, 'e');

    printf("Expected: abcd, got: %s\n", (char *)*vector_begin(v1));
    assert(equals_string((char *)*vector_begin(v1), "abcd"));
    printf("Expected:  , got: %s\n", (char *)*vector_at(v1, (size_t)1));
    assert(equals_string((char *)*vector_at(v1, (size_t)1), ""));
    printf("Expected: fg, got: %s\n", (char *)*vector_at(v1, (size_t)2));
    assert(equals_string((char *)*vector_at(v1, (size_t)2), "fg"));

    sstring_destroy(str1);
    vector_destroy(v1);

    // test 3.2
    str1 = cstr_to_sstring("This is a sentence.");
    vector *v2 = sstring_split(str1, ' ');

    printf("Expected: This, got: %s\n", (char *)*vector_begin(v2));
    assert(equals_string((char *)*vector_begin(v2), "This"));
    printf("Expected: is, got: %s\n", (char *)*vector_at(v2, (size_t)1));
    assert(equals_string((char *)*vector_at(v2, (size_t)1), "is"));
    printf("Expected: a, got: %s\n", (char *)*vector_at(v2, (size_t)2));
    assert(equals_string((char *)*vector_at(v2, (size_t)2), "a"));
    printf("Expected: sentence, got: %s\n", (char *)*vector_back(v2));
    assert(equals_string((char *)*vector_begin(v2), "This"));

    sstring_destroy(str1);
    vector_destroy(v2);


    // test 3.3 (edge case ?) -> Test SString Split Hard: Test sstring_split where the result have empty strings.
    str1 = cstr_to_sstring("Mom has a car!");
    v1 = sstring_split(str1, 'q');

    printf("Expected: Mom has a car!, got: %s\n", (char *)*vector_begin(v1));
    assert(equals_string((char *)*vector_begin(v1), "Mom has a car!"));

    sstring_destroy(str1);
    vector_destroy(v1);

    // test 3.4 (edge case ?) -> Test SString Split Hard: Test sstring_split where the result have empty strings.
    str1 = cstr_to_sstring("abc");
    v1 = sstring_split(str1, 'a');

    printf("Expected: , got: %s\n", (char *)*vector_begin(v1));
    assert(equals_string((char *)*vector_begin(v1), ""));

    printf("Expected: bc!, got: %s\n", (char *)*vector_at(v1, (size_t) 1));
    assert(equals_string((char *)*vector_at(v1, (size_t) 1), "bc"));

    sstring_destroy(str1);
    vector_destroy(v1);

    // test 3.5 (edge case ?) -> Test SString Split Hard: Test sstring_split where the result have empty strings.
    str1 = cstr_to_sstring("aaaa");
    v1 = sstring_split(str1, 'a');

    assert(equals_string((char *)*vector_begin(v1), ""));
    assert(equals_string((char *)*vector_at(v1, (size_t) 1), ""));
    assert(equals_string((char *)*vector_at(v1, (size_t) 2), ""));
    assert(equals_string((char *)*vector_at(v1, (size_t) 3), ""));

    sstring_destroy(str1);
    vector_destroy(v1);

    // HERE WAS THE ERROR!: test 3.6 (edge case ?) -> Test SString Split Hard: Test sstring_split where the result have empty strings.
    str1 = cstr_to_sstring("bca");
    v1 = sstring_split(str1, 'a');

    assert(equals_string((char *)*vector_at(v1, (size_t) 0), "bc"));
    assert(equals_string((char *)*vector_at(v1, (size_t) 1), ""));

    sstring_destroy(str1);
    vector_destroy(v1);

    // HERE WAS THE ERROR!: test 3.7 (edge case ?) -> Test SString Split Hard: Test sstring_split where the result have empty strings.
    str1 = cstr_to_sstring("abca");
    v1 = sstring_split(str1, 'a');

    assert(equals_string((char *)*vector_begin(v1), ""));
    assert(equals_string((char *)*vector_at(v1, (size_t) 1), "bc"));
    assert(equals_string((char *)*vector_at(v1, (size_t) 2), ""));

    sstring_destroy(str1);
    vector_destroy(v1);

    // test 4.1 & 4.2: sstring_split
    //  *
    //  * sstring *replace_me = cstr_to_sstring("This is a {} day, {}!");
    //  * sstring_substitute(replace_me, 18, "{}", "friend");
    //  * sstring_to_cstr(replace_me); // == "This is a {} day, friend!"
    //  * sstring_substitute(replace_me, 0, "{}", "good");
    //  * sstring_to_cstr(replace_me); // == "This is a good day, friend!"
    //  */

    // test 4.1 : ret -1
    sstr = cstr_to_sstring("Hi there, I'm Vlad!");
    size_t offset = 1;
    char *target = "Koshin"; // Not found sstr->str
    char *substitution = "Mara";

    assert(sstring_substitute(sstr, offset, target, substitution) == -1);
    sstring_destroy(sstr);

    // test 4.2 : ret "Hi there, I'm Mara"
    sstr = cstr_to_sstring("Hi there, I'm Vlad!");
    offset = 14;
    target = "Vlad"; // found!
    substitution = "Mara";

    int ans = sstring_substitute(sstr, offset, target, substitution);
    obtained =  sstring_to_cstr(sstr);
    printf("Expected: Hi there, I'm Mara!, got: %s\n", obtained);
    assert(ans == 0);
    free(obtained);
    sstring_destroy(sstr);

    // test 4.3 : ret -1 (some boundary testing, expected : 14 offset, but 15 offset is too much)
    sstr = cstr_to_sstring("Hi there, I'm Vlad!");
    offset = 15;
    target = "Vlad"; // found!
    substitution = "Mara";

    ans = sstring_substitute(sstr, offset, target, substitution);
    obtained =  sstring_to_cstr(sstr);
    printf("Expected (no modifications, initial string): Hi there, I'm Vlad!, got: %s\n", obtained);
    assert(ans == -1);
    free(obtained);
    sstring_destroy(sstr);

    sstring *replace_me = cstr_to_sstring("This is a {} day, {}!");
    int ans1 = sstring_substitute(replace_me, 18, "{}", "friend");
    assert(ans1 == 0);
    char *obt1 = sstring_to_cstr(replace_me); // == "This is a {} day, friend!"
    printf("obt1 is: %s\n", obt1);
    assert(equals_string(obt1, "This is a {} day, friend!"));
    free(obt1);

    int ans2 = sstring_substitute(replace_me, 0, "{}", "good");
    assert(ans2 == 0);
    char *obt2 = sstring_to_cstr(replace_me); // == "This is a good day, friend!"
    printf("obt2 is: %s\n", obt2);
    assert(equals_string(obt2, "This is a good day, friend!"));
    free(obt2);

    sstring_destroy(replace_me);
    

    // test 5: sstring_slice
    sstring *slice_me = cstr_to_sstring("1234567890");
    char *obt = sstring_slice(slice_me, 2, 5);// == "345" (don't forget about the null byte!)
    printf("Expected: 345, and got %s\n", obt);
    assert(equals_string(obt, "345"));
    sstring_destroy(slice_me);
    free(obt);

    // sstring_destroy is tested if Valgrind tells us that we don't leak memory.

    puts("All tests pass. Hooray!");
    return 0;
}
