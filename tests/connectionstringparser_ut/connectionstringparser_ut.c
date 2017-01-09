// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif 

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* s)
{
    free(s);
}

void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#include "azure_c_shared_utility/crt_abstractions.h"

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_bool.h"
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#undef ENABLE_MOCKS

#include "azure_c_shared_utility/connection_string_parser.h"

#include "real_map.h"
#include "real_string_tokenizer.h"
#include "real_strings.h"

TEST_DEFINE_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(MAP_RESULT, MAP_RESULT_VALUES);

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static const char* TEST_STRING_PAIR = "key1=value1";
STRING_HANDLE TEST_STRING_HANDLE_PAIR;
static const char* TEST_STRING_KEY = "key1=";
STRING_HANDLE TEST_STRING_HANDLE_KEY;
static const char* TEST_STRING_2_PAIR = "key1=value1;key2=value2";
STRING_HANDLE TEST_STRING_HANDLE_2_PAIR;
static const char* TEST_STRING_2_PAIR_SEMICOLON = "key1=value1;key2=value2;";
STRING_HANDLE TEST_STRING_HANDLE_2_PAIR_SEMICOLON;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(connectionstringparser_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(MAP_RESULT, MAP_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_TOKENIZER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_FILTER_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;
    REGISTER_STRING_TOKENIZER_GLOBAL_MOCK_HOOK;
    REGISTER_MAP_GLOBAL_MOCK_HOOK;
    
    TEST_STRING_HANDLE_PAIR = STRING_construct(TEST_STRING_PAIR);
    TEST_STRING_HANDLE_KEY = STRING_construct(TEST_STRING_KEY);
    TEST_STRING_HANDLE_2_PAIR = STRING_construct(TEST_STRING_2_PAIR);
    TEST_STRING_HANDLE_2_PAIR_SEMICOLON = STRING_construct(TEST_STRING_2_PAIR_SEMICOLON);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    STRING_delete(TEST_STRING_HANDLE_PAIR);
    STRING_delete(TEST_STRING_HANDLE_KEY);
    STRING_delete(TEST_STRING_HANDLE_2_PAIR);
    STRING_delete(TEST_STRING_HANDLE_2_PAIR_SEMICOLON);

    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* connectionstringparser_parse */

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_001: [connectionstringparser_parse shall parse all key value pairs from the connection_string passed in as argument and return a new map that holds the key/value pairs.]  */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_003: [connectionstringparser_parse shall create a STRING tokenizer to be used for parsing the connection string, by calling STRING_TOKENIZER_create.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_004: [connectionstringparser_parse shall start scanning at the beginning of the connection string.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_016: [2 STRINGs shall be allocated in order to hold the to be parsed key and value tokens.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_005: [The following actions shall be repeated until parsing is complete:] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_006: [connectionstringparser_parse shall find a token (the key of the key/value pair) delimited by the "=" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_007: [If STRING_TOKENIZER_get_next_token fails, parsing shall be considered complete.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_014: [After the parsing is complete the previously allocated STRINGs and STRING tokenizer shall be freed by calling STRING_TOKENIZER_destroy.] */
TEST_FUNCTION(connectionstringparser_parse_with_an_empty_string_yields_an_empty_map)
{
    // arrange
    STRING_HANDLE connectionString = STRING_new();
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(connectionString);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(connectionString)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(connectionString);

    // assert
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    STRING_delete(connectionString);
    Map_Destroy(result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_002: [If connection_string is NULL then connectionstringparser_parse shall fail and return NULL.] */
TEST_FUNCTION(connectionstringparser_parse_with_NULL_connection_string_fails)
{
    // arrange

    // act
    MAP_HANDLE result = connectionstringparser_parse(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_015: [If STRING_TOKENIZER_create fails, connectionstringparser_parse shall fail and return NULL.] */
TEST_FUNCTION(when_creating_the_string_tokenizer_fails_then_connectionstringparser_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR))
        .SetReturn((STRING_TOKENIZER_HANDLE)NULL);

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);

    // cleanup
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_017: [If allocating the STRINGs fails connectionstringparser_parse shall fail and return NULL.] */
TEST_FUNCTION(when_allocating_the_key_token_string_fails_then_connectionstringparser_fails)
{
    // arrange
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn((STRING_HANDLE)NULL);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_017: [If allocating the STRINGs fails connectionstringparser_parse shall fail and return NULL.] */
TEST_FUNCTION(when_allocating_the_value_token_string_fails_then_connectionstringparser_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn((STRING_HANDLE)NULL);
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_018: [If creating the result map fails, then connectionstringparser_parse shall return NULL.] */
TEST_FUNCTION(when_allocating_the_result_map_fails_then_connectionstringparser_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn((MAP_HANDLE)NULL);
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_001: [connectionstringparser_parse shall parse all key value pairs from the connection_string passed in as argument and return a new map that holds the key/value pairs.]  */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_003: [connectionstringparser_parse shall create a STRING tokenizer to be used for parsing the connection string, by calling STRING_TOKENIZER_create.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_004: [connectionstringparser_parse shall start scanning at the beginning of the connection string.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_016: [2 STRINGs shall be allocated in order to hold the to be parsed key and value tokens.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_005: [The following actions shall be repeated until parsing is complete:] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_006: [connectionstringparser_parse shall find a token (the key of the key/value pair) delimited by the "=" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_007: [If STRING_TOKENIZER_get_next_token fails, parsing shall be considered complete.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_014: [After the parsing is complete the previously allocated STRINGs and STRING tokenizer shall be freed by calling STRING_TOKENIZER_destroy.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_008: [connectionstringparser_parse shall find a token (the value of the key/value pair) delimited by the ";" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_010: [The key and value shall be added to the result map by using Map_Add.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_011: [The C strings for the key and value shall be extracted from the previously parsed STRINGs by using STRING_c_str.] */
TEST_FUNCTION(connectionstringparser_parse_with_a_key_value_pair_adds_it_to_the_result_map)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_PAIR +5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(IGNORED_PTR_ARG, "key1", "value1")).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(5));
    STRICT_EXPECTED_CALL(gballoc_malloc(7));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, "value1", Map_GetValueFromKey(result, "key1"));

    // cleanup
    Map_Destroy(result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_009: [If STRING_TOKENIZER_get_next_token fails, connectionstringparser_parse shall fail and return NULL (freeing the allocated result map).] */
TEST_FUNCTION(when_getting_the_value_token_fails_then_connectionstringparser_parse_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_KEY);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_KEY)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_KEY, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(Map_Destroy(map));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_KEY);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_019: [If the key length is zero then connectionstringparser_parse shall fail and return NULL (freeing the allocated result map).] */
TEST_FUNCTION(when_the_key_is_zero_length_then_connectionstringparser_parse_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR));
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE_PAIR));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, key, "=")).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, value, ";")).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_PAIR + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key)).SetReturn("");
    STRICT_EXPECTED_CALL(Map_Destroy(map));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_012: [If Map_Add fails connectionstringparser_parse shall fail and return NULL (freeing the allocated result map).] */
TEST_FUNCTION(when_adding_the_key_value_pair_to_the_map_fails_then_connectionstringparser_parse_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_PAIR + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(map, "key1", "value1")).SetReturn((MAP_RESULT)MAP_INVALIDARG);
    STRICT_EXPECTED_CALL(Map_Destroy(map));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_013: [If STRING_c_str fails then connectionstringparser_parse shall fail and return NULL (freeing the allocated result map).] */
TEST_FUNCTION(when_getting_the_C_string_for_the_key_fails_then_connectionstringparser_parse_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR));
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_STRING_HANDLE_PAIR));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new()).SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, key, "=")).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(IGNORED_PTR_ARG, value, ";")).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_PAIR + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(Map_Destroy(map));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(IGNORED_PTR_ARG)).IgnoreArgument(1);

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_013: [If STRING_c_str fails then connectionstringparser_parse shall fail and return NULL (freeing the allocated result map).] */
TEST_FUNCTION(when_getting_the_C_string_for_the_value_fails_then_connectionstringparser_parse_fails)
{
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_PAIR + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value)).SetReturn((const char*)NULL);
    STRICT_EXPECTED_CALL(Map_Destroy(map));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_001: [connectionstringparser_parse shall parse all key value pairs from the connection_string passed in as argument and return a new map that holds the key/value pairs.]  */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_003: [connectionstringparser_parse shall create a STRING tokenizer to be used for parsing the connection string, by calling STRING_TOKENIZER_create.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_004: [connectionstringparser_parse shall start scanning at the beginning of the connection string.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_016: [2 STRINGs shall be allocated in order to hold the to be parsed key and value tokens.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_005: [The following actions shall be repeated until parsing is complete:] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_006: [connectionstringparser_parse shall find a token (the key of the key/value pair) delimited by the "=" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_007: [If STRING_TOKENIZER_get_next_token fails, parsing shall be considered complete.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_014: [After the parsing is complete the previously allocated STRINGs and STRING tokenizer shall be freed by calling STRING_TOKENIZER_destroy.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_008: [connectionstringparser_parse shall find a token (the value of the key/value pair) delimited by the ";" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_010: [The key and value shall be added to the result map by using Map_Add.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_011: [The C strings for the key and value shall be extracted from the previously parsed STRINGs by using STRING_c_str.] */
TEST_FUNCTION(connectionstringparser_parse_with_2_key_value_pairs_adds_them_to_the_result_map)
{
    // arrange
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_2_PAIR);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_2_PAIR)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);

    // 1st kvp
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_2_PAIR, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_2_PAIR + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(map, "key1", "value1"));
    STRICT_EXPECTED_CALL(gballoc_malloc(5));
    STRICT_EXPECTED_CALL(gballoc_malloc(7));

    // 2st kvp
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, (TEST_STRING_2_PAIR + 12), 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_2_PAIR + 17), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(map, "key2", "value2"));
    STRICT_EXPECTED_CALL(gballoc_malloc(5));
    STRICT_EXPECTED_CALL(gballoc_malloc(7));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_2_PAIR);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, map, result);
    ASSERT_ARE_EQUAL(char_ptr, "value1", Map_GetValueFromKey(result, "key1"));
    ASSERT_ARE_EQUAL(char_ptr, "value2", Map_GetValueFromKey(result, "key2"));

    // cleanup
    Map_Destroy(result);

}

/* Tests_SRS_CONNECTIONSTRINGPARSER_01_001: [connectionstringparser_parse shall parse all key value pairs from the connection_string passed in as argument and return a new map that holds the key/value pairs.]  */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_003: [connectionstringparser_parse shall create a STRING tokenizer to be used for parsing the connection string, by calling STRING_TOKENIZER_create.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_004: [connectionstringparser_parse shall start scanning at the beginning of the connection string.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_016: [2 STRINGs shall be allocated in order to hold the to be parsed key and value tokens.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_005: [The following actions shall be repeated until parsing is complete:] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_006: [connectionstringparser_parse shall find a token (the key of the key/value pair) delimited by the "=" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_007: [If STRING_TOKENIZER_get_next_token fails, parsing shall be considered complete.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_014: [After the parsing is complete the previously allocated STRINGs and STRING tokenizer shall be freed by calling STRING_TOKENIZER_destroy.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_008: [connectionstringparser_parse shall find a token (the value of the key/value pair) delimited by the ";" character, by calling STRING_TOKENIZER_get_next_token.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_010: [The key and value shall be added to the result map by using Map_Add.] */
/* Tests_SRS_CONNECTIONSTRINGPARSER_01_011: [The C strings for the key and value shall be extracted from the previously parsed STRINGs by using STRING_c_str.] */
TEST_FUNCTION(connectionstringparser_parse_with_2_key_value_pairs_ended_with_semicolon_adds_them_to_the_result_map)
{
    // arrange
    // arrange
    STRING_HANDLE key = STRING_new();
    STRING_HANDLE value = STRING_new();
    MAP_HANDLE map = Map_Create(NULL);
    STRING_TOKENIZER_HANDLE tokens = STRING_TOKENIZER_create(TEST_STRING_HANDLE_2_PAIR_SEMICOLON);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_create(TEST_STRING_HANDLE_2_PAIR_SEMICOLON)).SetReturn(tokens);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(key);
    STRICT_EXPECTED_CALL(STRING_new())
        .SetReturn(value);
    STRICT_EXPECTED_CALL(Map_Create(NULL)).SetReturn(map);

    // 1st kvp
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, TEST_STRING_2_PAIR_SEMICOLON, 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_2_PAIR_SEMICOLON + 5), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(map, "key1", "value1"));
    STRICT_EXPECTED_CALL(gballoc_malloc(5));
    STRICT_EXPECTED_CALL(gballoc_malloc(7));

    // 2st kvp
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_copy_n(key, (TEST_STRING_2_PAIR_SEMICOLON + 12), 4));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, value, ";"));
    STRICT_EXPECTED_CALL(STRING_copy_n(value, (TEST_STRING_2_PAIR_SEMICOLON + 17), 6));
    STRICT_EXPECTED_CALL(STRING_c_str(key));
    STRICT_EXPECTED_CALL(STRING_c_str(value));
    STRICT_EXPECTED_CALL(Map_Add(map, "key2", "value2"));
    STRICT_EXPECTED_CALL(gballoc_malloc(5));
    STRICT_EXPECTED_CALL(gballoc_malloc(7));

    STRICT_EXPECTED_CALL(STRING_TOKENIZER_get_next_token(tokens, key, "="));
    STRICT_EXPECTED_CALL(STRING_delete(value));
    STRICT_EXPECTED_CALL(STRING_delete(key));
    STRICT_EXPECTED_CALL(STRING_TOKENIZER_destroy(tokens));

    // act
    MAP_HANDLE result = connectionstringparser_parse(TEST_STRING_HANDLE_2_PAIR_SEMICOLON);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, map, result);
    ASSERT_ARE_EQUAL(char_ptr, "value1", Map_GetValueFromKey(result, "key1"));
    ASSERT_ARE_EQUAL(char_ptr, "value2", Map_GetValueFromKey(result, "key2"));

    // cleanup
    Map_Destroy(result);
}

END_TEST_SUITE(connectionstringparser_ut)