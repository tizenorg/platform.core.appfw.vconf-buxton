/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Copyright (C) 2014 Intel Corporation
 *
 * Author: José Bollo <jose.bollo@open.eurogiciel.org>
 * Author: Hakjoo Ko <hakjoo.ko@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VCONF_BUXTON_H__
#define __VCONF_BUXTON_H__

#define VCONF_BUXTON  "1.0"

/**
 * @addtogroup APPLICATION_FRAMEWORK
 * @{
 *
 * @defgroup    VCONF VConf
 * @author      Sangjung Woo (sangjung.woo@samsung.com)
 * @version     0.2
 * @brief       A library for reading/writing Configuration Data
 *
 * @section Header To use Them:
 * @code
 * #include <vconf.h>
 * @endcode
 *
 * @section Properties
 * - Convenient API
 * - Guarantee Transaction(db backend only)
 * - Apply Key-List concept
 * - Changeable Backend
 * - Simple Notification based on inotify
 *
 * @section Backend key has below backend.
 * - db => use libsqlfs ex) db/a/b
 * \n Lowest speed, highest robustness, correctly sync
 * - memory => use tmpfs ex) memory/a/b
 * \n Highest speed, volitile
 * - file => use basic file system(not support atomicity) ex) file/a/b
 * \n Higher speed, lower robustness(Not guarantee atomicity)
 *
 * @section example Simple Example
 * @code
 #include <stdio.h>
 #include <vconf.h>

 const char *key1_name="db/test/key1";

 int main(int argc, char **argv)
 {
   int key1_value;

   if(vconf_set_int(key1_name,1))
      fprintf(stderr, "vconf_set_int FAIL\n");
   else
      printf("vconf_set_int OK\n");

   if(vconf_get_int(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_int FAIL\n");
   else
      printf("vconf_get_int OK(key1 value is %d)\n", key1_value);

   return 0;
 }
 * @endcode
 *
 */

/**
 * @addtogroup VCONF
 * @{
 */

#include "vconf-buxton-keys.h"

#ifdef __cplusplus
extern          "C" {
#endif

/**
 * @brief Enumeration for uses of vconf_get().
 * @since_tizen 2.3
 * @see vconf_get()
 */
    enum get_option_t {
	VCONF_GET_KEY = 0,
	    /**< get only keys */
	VCONF_GET_ALL,
	    /**< get keys and directorys */
	VCONF_GET_DIR,
	   /**< get only directorys */
	VCONF_REFRESH_ONLY,
	    /**< get only keys */
	VCONF_GET_KEY_REC,
	    /**< get only keys recursively */
	VCONF_GET_ALL_REC,
	    /**< get keys and directorys recursively */
	VCONF_GET_DIR_REC
	   /**< get only directorys recursively */
    };

/**
 * @brief  Enumeration for Definition for Enumeration type.
 * @since_tizen 2.3
 *
 */
typedef enum get_option_t get_option_t;

/**
 * @brief Enumeration for vconf_t.
 * @since_tizen 2.3
 *
 */
    enum vconf_t {
	VCONF_TYPE_NONE = 0,
		   /**< Vconf none type for Error detection */
	VCONF_TYPE_STRING = 40,
		   /**< Vconf string type */
	VCONF_TYPE_INT = 41,
		   /**< Vconf integer type */
	VCONF_TYPE_DOUBLE = 42,
		   /**< Vconf double type */
	VCONF_TYPE_BOOL = 43,
		   /**< Vconf boolean type */
	VCONF_TYPE_DIR
		   /**< Vconf directory type */
    };


/**
 * @brief The structure type for an opaque type. It must be used via accessor functions.
 * @since_tizen 2.3
 *
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_dbl()
 * @see vconf_keynode_get_int()
 * @see vconf_keynode_get_str()
 */
    typedef struct _keynode_t keynode_t;

/**
 * @brief The structure type for opaque type. It must be used via accessor functions.
 * @since_tizen 2.3
 *
 * @see vconf_keylist_new()
 * @see vconf_keylist_free()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 * @see vconf_keylist_lookup()
 * @see vconf_keylist_nextnode()
 * @see vconf_keylist_rewind()
 */
    typedef struct _keylist_t keylist_t;


/**
 * @brief  Called when the key is set handle.
 * @details  This is the signature of a callback function added with vconf_notify_key_changed() handle.
 *
 * @since_tizen 2.3
 *
 * @see keynode_t
 */
    typedef void (*vconf_callback_fn) (keynode_t *node, void *user_data);

/************************************************
 * keynode handling APIs                        *
 ************************************************/

/**
 * @brief Gets the key name of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The key name of the keynode
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_str()
 * @see vconf_keynode_get_int()
 * @see vconf_keynode_get_dbl()
 * @see keynode_t
 * @see vconf_t
 */
    const char *vconf_keynode_get_name(keynode_t *keynode);

/**
 * @brief Gets the value type of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The type of the keynode
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_str()
 * @see vconf_keynode_get_int()
 * @see vconf_keynode_get_dbl()
 * @see keynode_t
 * @see vconf_t
 */
    int vconf_keynode_get_type(keynode_t *keynode);

/**
 * @brief Gets the integer value of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The integer value,
 *          otherwise @c 0 if no value is obtained
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_str()
 * @see vconf_keynode_get_dbl()
 * @see keynode_t
 * @see vconf_t
 */
    int vconf_keynode_get_int(keynode_t *keynode);

/**
 * @brief Gets the double value of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The double value,
 *          otherwise @c 0.0 if no value is obtained
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_str()
 * @see vconf_keynode_get_int()
 * @see keynode_t
 * @see vconf_t
 */
    double vconf_keynode_get_dbl(keynode_t *keynode);

/**
 * @brief Gets the boolean value of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The boolean value,
 *          otherwise @c -1 on error \n
 *          Integer value  @c 1 is 'True', and @c 0 is 'False'.
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_str()
 * @see vconf_keynode_get_int()
 * @see vconf_keynode_get_dbl()
 * @see keynode_t
 * @see vconf_t
 */
    int vconf_keynode_get_bool(keynode_t *keynode);

/**
 * @brief Gets the string value of a keynode.
 *
 * @since_tizen 2.3
 *
 * @param[in] keynode The Key
 *
 * @return  The string value,
 *          otherwise @c NULL if no value is obtained
 *
 * @see vconf_notify_key_changed()
 * @see vconf_keynode_get_name()
 * @see vconf_keynode_get_bool()
 * @see vconf_keynode_get_type()
 * @see vconf_keynode_get_int()
 * @see vconf_keynode_get_dbl()
 * @see keynode_t
 * @see vconf_t
 */
    char *vconf_keynode_get_str(keynode_t *keynode);


/************************************************
 * keylist handling APIs
 ************************************************/

/**
 * @brief Allocates, initializes and returns a new keylist object.
 * @details You must release the return value keylist_t* pointer using vconf_keylist_free().
 *
 * @since_tizen 2.3
 *
 * @return  The pointer of New keylist,
 *          otherwise @c NULL on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_new()
 * @see vconf_keylist_free()
 */
    keylist_t *vconf_keylist_new(void);

/**
 * @brief Moves the current keynode position to the first item.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 *
 * @return  @c 0 on success,
 *          otherwise -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_nextnode()
 * @see vconf_keylist_rewind()
 * @see vconf_keylist_nextnode()
 *
 * @par example
 * @code
    int r =0;
    keylist_t* pKeyList = NULL;
    pKeyList = vconf_keylist_new();

    r = vconf_get(pKeyList, KEY_PARENT, VCONF_GET_KEY);
    if (r) {
    tet_infoline("vconf_get() failed in positive test case");
    tet_result(TET_FAIL);
    return;
    }

    vconf_keylist_nextnode(pKeyList);
    vconf_keylist_nextnode(pKeyList);

    // Move first position from KeyList
    r = vconf_keylist_rewind(pKeyList);
    if (r<0) {
    tet_infoline("vconf_keylist_rewind() failed in positive test case");
    tet_result(TET_FAIL);
    return;
    }

    while(vconf_keylist_nextnode(pKeyList)) ;
 * @endcode
 */
    int vconf_keylist_rewind(keylist_t *keylist);

/**
 * @brief Destroys a keylist.
 * @details After calling vconf_keylist_new(), you must call this function to release internal memory.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_new()
 */
    int vconf_keylist_free(keylist_t *keylist);

/**
 * @brief Looks for a keynode contained in a keylist that matches the keyname.
 *
 * @since_tizen 2.3
 *
 * @param[in]  keylist      The Key List
 * @param[in]  keyname      The key to find
 * @param[out] return_node  The pointer of the keynode to set
 *
 * @return  The type of the found key that is vconf_t enumeration value
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see keynode_t
 * @see vconf_t
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

int main()
{
    int r = 0;
    int nResult = 0;
    keylist_t* pKeyList = NULL;
    keynode_t *pKeyNode;

    pKeyList = vconf_keylist_new();
    r = vconf_get(pKeyList, KEY_PARENT, VCONF_GET_KEY);
    if (r<0) {
        printf("vconf_get() failed in positive test case");
        return -1;
    }

    r = vconf_keylist_lookup(pKeyList, KEY_02, &pKeyNode);
    if (r<0) {
        printf("vconf_get() failed in positive test case");
        return -1;
    }

    nResult = vconf_keynode_get_int(pKeyNode);
    if(nResult !=KEY_02_INT_VALUE)
    {
        printf("vconf_get() failed in positive test case");
        return -1;

    }

    vconf_keylist_free(pKeyList);
    return 0;
}
 * @endcode
 */
    int vconf_keylist_lookup(keylist_t *keylist, const char *keyname,
                 keynode_t **return_node);

/**
 * @brief Gets the next key in a keylist.
 * @details The next key is known by the keylist internal cursor.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 *
 * @return  The next Keynode,
 *          otherwise @c NULL on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_rewind()
 * @see vconf_keylist_nextnode()
 * @see keynode_t
 */
    keynode_t *vconf_keylist_nextnode(keylist_t *keylist);

/**
 * @brief Appends a new keynode containing an integer value to a keylist.
 * @details If the same keyname exists, the keynode will change.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 * @param[in] keyname  The key
 * @param[in] value    The integer value
 *
 * @return  The number of keynode included in the keylist,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 */
    int vconf_keylist_add_int(keylist_t *keylist, const char *keyname,
                  const int value);

/**
 * @brief Appends a new keynode containing a boolean value to a keylist.
 * @details If the same keyname exist, the keynode will change.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 * @param[in] keyname  The key
 * @param[in] value    The boolean value
 *
 * @return  The number of keynodes included in the keylist,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 */
    int vconf_keylist_add_bool(keylist_t *keylist, const char *keyname,
                   const int value);

/**
 * @brief Appends a new keynode containing a double value to a keylist.
 * @details If the same keyname exist, the keynode will change.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 * @param[in] keyname  The key
 * @param[in] value    The double value
 *
 * @return  The number of the keynodes included in the keylist,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 */
    int vconf_keylist_add_dbl(keylist_t *keylist, const char *keyname,
                  const double value);

/**
 * @brief Appends a new keynode containing a string to a keylist.
 * @details If the same keyname exist, the keynode will change.
 *
 * @since_tizen 2.3
 *
 * @remarks The size limit of value is 4K.
 *
 * @param[in] keylist  The Key List
 * @param[in] keyname  The key
 * @param[in] value    The pointer of string value
 *
 * @return  The number of keynodes included in the keylist,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 */
    int vconf_keylist_add_str(keylist_t *keylist, const char *keyname,
                  const char *value);

/**
 * @brief Appends a new keynode to a keylist without a value.
 * @details Uses for vconf_get().
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The Key List
 * @param[in] keyname  The key
 *
 * @return  The number of the keynodes included in the keylist,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 */
    int vconf_keylist_add_null(keylist_t *keylist, const char *keyname);

/**
 * @brief Removes the keynode that matches the given keyname.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist The keylist containing the keyname
 * @param[in] keyname The key
 *
 * @return  @c 0 on success,
 *          @c -1 if invalid parameter),
 *          otherwise @c -2 (Not exist keyname in keylist) on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 */
    int vconf_keylist_del(keylist_t *keylist, const char *keyname);

/************************************************
 * setting APIs                                 *
 ************************************************/

/**
 * @brief Sets the keys included in a keylist.
 * @details If you use DB backend, the keylist is handled as one transaction.
 *
 * @since_tizen 2.3
 *
 * @param[in] keylist  The keylist which should contain changed keys
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_set()
 * @see vconf_get()
 * @see vconf_keylist_add_int()
 * @see vconf_keylist_add_str()
 * @see vconf_keylist_add_dbl()
 * @see vconf_keylist_add_bool()
 * @see vconf_keylist_del()
 * @see vconf_keylist_add_null()
 *
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

int main()
{
   keylist_t *kl=NULL;
   const char *keyname_list[3]={"db/test/key1", "db/test/key2", "db/test/key3"};

   // Transaction Test(all or nothing is written)
   kl = vconf_keylist_new();

   vconf_keylist_add_int(kl, keyname_list[0], 1);
   vconf_keylist_add_str(kl, keyname_list[1], "transaction Test");
   vconf_keylist_add_dbl(kl, keyname_list[2], 0.3);
   if(vconf_set(kl))
      fprintf(stderr, "nothing is written\n");
   else
      printf("everything is written\n");

   vconf_keylist_free(kl);

   // You can set items which have different backend.
   kl = vconf_keylist_new();

   vconf_keylist_add_int(kl, "memory/a/xxx1", 4);
   vconf_keylist_add_str(kl, "file/a/xxx2", "test 3");
   vconf_keylist_add_dbl(kl, "db/a/xxx3", 0.3);
   vconf_set(kl)

   vconf_keylist_free(kl);
   return 0;
}
 * @endcode
 */
    int vconf_set(keylist_t *keylist);

/**
 * @brief Sets the integer value of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key  The key
 * @param[in]   intval  The integer value to set \n
 *                      @c 0 is also allowed as a value.
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_set_bool()
 * @see vconf_set_dbl()
 * @see vconf_set_str()
 */
    int vconf_set_int(const char *in_key, const int intval);

/**
 * @brief Sets the boolean value of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key   The key
 * @param[in]   boolval  The Boolean value( @c 1 or @c 0) to set
 *                       Integer value @c 1 is 'True', and @c 0 is 'False'.
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_set_int()
 * @see vconf_set_dbl()
 * @see vconf_set_str()
 *
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

 const char *key1_name="memory/test/key1";

 int main(int argc, char **argv)
 {
   int key1_value;

   if(vconf_set_bool(key1_name, 1))
      fprintf(stderr, "vconf_set_bool FAIL\n");
   else
      printf("vconf_set_bool OK\n");

   if(vconf_get_bool(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_bool FAIL\n");
   else
      printf("vconf_get_bool OK(key1 value is %d)\n", key1_value);

   return 0;
 }
 * @endcode
 */
    int vconf_set_bool(const char *in_key, const int boolval);

/**
 * @brief Sets the double value of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key  The key
 * @param[in]   dblval  The double value to set \n
 *                      @c 0.0 is also allowed as a value.
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_set_int()
 * @see vconf_set_bool()
 * @see vconf_set_str()
 */
    int vconf_set_dbl(const char *in_key, const double dblval);

/**
 * @brief Sets the string value of the given key.
 *
 * @since_tizen 2.3
 *
 * @remarks The size limit of value is 4K.
 *
 * @param[in]   in_key  The key
 * @param[in]   strval  The string value to set
 *
 * @return  @c 0 on success,
 *          otherwise -1 on error
 *
 * @see vconf_set_bool()
 * @see vconf_set_dbl()
 * @see vconf_set_int()
 */
    int vconf_set_str(const char *in_key, const char *strval);

/**
 * @brief Gets the keys or subdirectory in in_parentDIR.
 * @details If the keylist has any key information, vconf only retrieves the keys.
 *          This is not recursive.
 *
 * @since_tizen 2.3
 *
 * @param[in]    keylist       The keylist created by vconf_keylist_new()
 * @param[in]    in_parentDIR  The parent DIRECTORY of needed keys
 * @param[in]    option        The options \n
 *                             VCONF_GET_KEY|VCONF_GET_DIR|VCONF_GET_ALL
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

int main()
{
   keylist_t *kl=NULL;
   keynode_t *temp_node;
   const char *vconfkeys1="db/test/key1";
   const char *parent_dir="db/test";

   kl = vconf_keylist_new();
   if(vconf_get(kl, parent_dir, 0))
      fprintf(stderr, "vconf_get FAIL(%s)", vconfkeys1);
   else
      printf("vconf_get OK(%s)", vconfkeys1);

   while((temp_node = vconf_keylist_nextnode(kl))) {
      switch(vconf_keynode_get_type(temp_node)) {
    case VCONF_TYPE_INT:
        printf("key = %s, value = %d\n",
            vconf_keynode_get_name(temp_node), vconf_keynode_get_int(temp_node));
        break;
    case VCONF_TYPE_BOOL:
        printf("key = %s, value = %d\n",
            vconf_keynode_get_name(temp_node), vconf_keynode_get_bool(temp_node));
        break;
    case VCONF_TYPE_DOUBLE:
        printf("key = %s, value = %f\n",
            vconf_keynode_get_name(temp_node), vconf_keynode_get_dbl(temp_node));
        break;
    case VCONF_TYPE_STRING:
        printf("key = %s, value = %s\n",
            vconf_keynode_get_name(temp_node), vconf_keynode_get_str(temp_node));
        break;
    default:
        printf("Unknown Type\n");
      }
   }
   vconf_keylist_free(kl);
}
 * @endcode
 */
    int vconf_get(keylist_t *keylist, const char *in_parentDIR, get_option_t option);

/**
 * @brief Gets the integer value of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key  The key
 * @param[out]  intval  The output buffer
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_get_bool()
 * @see vconf_get_dbl()
 * @see vconf_get_str()
 *
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

const char *key1_name="db/test/key1";

int main(int argc, char **argv)
{
   int key1_value;

   if(vconf_set_int(key1_name,1))
      fprintf(stderr, "vconf_set_int FAIL\n");
   else
      printf("vconf_set_int OK\n");

   if(vconf_get_int(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_int FAIL\n");
   else
      printf("vconf_get_int OK(key1 value is %d)\n", key1_value);

   return 0;
}
 * @endcode
 */
    int vconf_get_int(const char *in_key, int *intval);

/**
 * @brief Gets the boolean value (@c 1 or @c 0) of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key   The key
 * @param[out]  boolval  The output buffer
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_get_int()
 * @see vconf_get_dbl()
 * @see vconf_get_str()
 */
    int vconf_get_bool(const char *in_key, int *boolval);

/**
 * @brief Gets the double value of the given key.
 *
 * @since_tizen 2.3
 *
 * @param[in]  in_key  The key
 * @param[out] dblval  The output buffer
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_get_int()
 * @see vconf_get_bool()
 * @see vconf_get_str()
 */
    int vconf_get_dbl(const char *in_key, double *dblval);

/**
 * @brief Gets the string value of the given key.
 * @details You have to free this returned value.
 *
 * @since_tizen 2.3
 *
 * @param[in] in_key  The key
 *
 * @return  The allocated pointer of key value on success,
 *          otherwise @c NULL on error
 *
 * @see vconf_get_int()
 * @see vconf_get_dbl()
 * @see vconf_get_bool()
 *
 * @par example
 * @code
   #include <stdio.h>
   #include <vconf.h>

   char *get_str=vconf_get_str("db/test/test1");
   if(get_str) {
      printf("vconf_get_str OK(value = %s)", get_str);
      free(get_str);
   }else
      fprintf(stderr, "vconf_get_str FAIL");
 * @endcode
 */
    char *vconf_get_str(const char *in_key);

/**
 * @brief Deletes the given key from the backend system.
 *
 * @since_tizen 2.3
 *
 * @param[in] in_key  The key
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 */
    int vconf_unset(const char *in_key);

/**
 * @brief Synchronizes the given key (only file backend) with the storage device.
 *
 * @since_tizen 2.3
 *
 * @param[in] in_key  The key
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @par example
 * @code
 if(vconf_set_int("file/test/key1",1))
    fprintf(stderr, "vconf_set_int FAIL\n");
 else {
    printf("vconf_set_int OK\n");
    vconf_sync_key("file/test/key1");
 }
 * @endcode
 */
    int vconf_sync_key(const char *in_key);

/**
 * @brief Deletes all keys and directories below the given directory from the backend system.
 *
 * @since_tizen 2.3
 *
 * @param[in] in_dir  The directory name for removal
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @par example
 * @code
   vconf_set_int("db/test/key1",1);
   vconf_set_int("db/test/test1/key1",1);
   vconf_set_int("db/test/test2/key1",1);
   vconf_set_int("db/test/key2",1);

   if(vconf_unset_recursive("db/test"))
      fprintf(stderr, "vconf_unset_recursive FAIL\n");
   else
      printf("vconf_unset_recursive OK(deleted db/test\n");

 * @endcode
 */
    int vconf_unset_recursive(const char *in_dir);

/**
 * @brief Adds a change callback for the given key, which is called when the key is set or unset.
 * @details The changed information (#keynode_t) of the key is delivered to #vconf_callback_fn,
 *          or if the key is deleted, the @link #keynode_t keynode @endlink has #VCONF_TYPE_NONE as type.
 *
 * @details Multiple vconf_callback_fn functions may exist for one key.
 *
 * @details The callback is issued in the context of the glib main loop.
 *
 * @since_tizen 2.3
 *
 * @remarks: This callback mechanism DOES NOT GUARANTEE consistency of data change. For example,
 *           When you have a callback for a certain key, assume that two or more processes are trying to
 *           change the value of the key competitively. In this case, the callback function will always
 *           get the 'CURRENT' value, not the value which raised the notification and caused the callback call.
 *           So, do not use vconf callback when competitive write for a key is happening. In such case, use
 *           socket-based IPC (dbus or something else) instead.
 *
 * @param[in] in_key     The key
 * @param[in] cb         The callback function
 * @param[in] user_data  The callback data
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see vconf_ignore_key_changed
 *
 * @par example
 * @code
 void test_cb(keynode_t *key, void* data)
 {
    switch(vconf_keynode_get_type(key))
    {
       case VCONF_TYPE_INT:
    printf("key = %s, value = %d(int)\n",
        vconf_keynode_get_name(key), vconf_keynode_get_int(key));
    break;
       case VCONF_TYPE_BOOL:
    printf("key = %s, value = %d(bool)\n",
        vconf_keynode_get_name(key), vconf_keynode_get_bool(key));
    break;
       case VCONF_TYPE_DOUBLE:
    printf("key = %s, value = %f(double)\n",
        vconf_keynode_get_name(key), vconf_keynode_get_dbl(key));
    break;
       case VCONF_TYPE_STRING:
    printf("key = %s, value = %s(string)\n",
        vconf_keynode_get_name(key), vconf_keynode_get_str(key));
    break;
       default:
    fprintf(stderr, "Unknown Type(%d)\n", vconf_keynode_get_type(key));
    break;
    }
    return;
 }

 int main()
 {
    int i;
    GMainLoop *event_loop;

    g_type_init();

    vconf_notify_key_changed("db/test/test1", test_cb, NULL);

    event_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(event_loop);

    vconf_ignore_key_changed("db/test/test1", test_cb);
    return 0;
 }
 * @endcode
 */
    int vconf_notify_key_changed(const char *in_key, vconf_callback_fn cb,
                     void *user_data);

/**
 * @brief Removes a change callback for the given key,
 *        which was added by vconf_notify_key_changed().
 *
 * @since_tizen 2.3
 *
 * @param[in]   in_key  The key
 * @param[in]   cb      The callback function
 *
 * @return @c 0 on success,
 *         otherwise @c -1 on error
 *
 * @see vconf_notify_key_changed()
 */
    int vconf_ignore_key_changed(const char *in_key, vconf_callback_fn cb);

/**
 * This function sorts the list in alphabetical order (with LANG=C)
 * @param[in] keylist Key List
 * @return 0 if done, -1 on error
 * @pre None
 * @post None
 * @remarks None
 */
    int             vconf_keylist_sort(keylist_t * keylist);

/**
 * This function retrieves the keys or subdirectory in in_parentDIR.<br>
 * @param[in]	keylist	keylist created by vconf_keylist_new(), MUST be empty
 * @param[in]	in_parentDIR parent DIRECTORY of needed keys
 * @param[in]	option VCONF_GET_KEY|VCONF_GET_DIR|VCONF_GET_ALL|VCONF_GET_KEY_REC|VCONF_GET_DIR_REC|VCONF_GET_ALL_REC
 * @return 0 on success, -1 on error
 * @pre None
 * @post None
 * @remkar None
 * @par example
 * @code
#include <stdio.h>
#include <vconf.h>

int main()
{
   keylist_t *kl=NULL;
   keynode_t *temp_node;
   const char *vconfkeys1="db/test/key1";
   const char *parent_dir="db/test";

   kl = vconf_keylist_new();
   if(vconf_scan(kl, parent_dir, VCONF_GET_KEY))
      fprintf(stderr, "vconf_get FAIL(%s)", vconfkeys1);
   else
      printf("vconf_get OK(%s)", vconfkeys1);

   while((temp_node = vconf_keylist_nextnode(kl))) {
      switch(vconf_keynode_get_type(temp_node)) {
	case VCONF_TYPE_INT:
		printf("key = %s, value = %d\n",
			vconf_keynode_get_name(temp_node), vconf_keynode_get_int(temp_node));
		break;
	case VCONF_TYPE_BOOL:
		printf("key = %s, value = %d\n",
			vconf_keynode_get_name(temp_node), vconf_keynode_get_bool(temp_node));
		break;
	case VCONF_TYPE_DOUBLE:
		printf("key = %s, value = %f\n",
			vconf_keynode_get_name(temp_node), vconf_keynode_get_dbl(temp_node));
		break;
	case VCONF_TYPE_STRING:
		printf("key = %s, value = %s\n",
			vconf_keynode_get_name(temp_node), vconf_keynode_get_str(temp_node));
		break;
	default:
		printf("Unknown Type\n");
      }
   }
   vconf_keylist_free(kl);
}
 * @endcode
 */
    int             vconf_scan(keylist_t * keylist,
			       const char *in_parentDIR,
			       get_option_t option);

/**
 * This function reads the database to refresh the values of
 * the keys in 'keylist'.
 * @param[in]	keylist	the keylist whose values have to be refreshed
 * @return 0 on success, -1 on error
 * @see vconf_keylist_lookup, vconf_keylist_nextnode, vconf_keylist_rewind
 */
    int             vconf_refresh(keylist_t * keylist);

/**
 * This function set the smack label of its keys
 * @param[in] keylist Key List
 * @param[in] label The label to set
 * @return 0 if done, -1 on error
 * @pre None
 * @post None
 * @remarks None
 */

    int             vconf_set_labels(keylist_t * keylist, const char *label);

/**
 * This function set the smack label of its keys
 * @param[in] keylist Key List
 * @param[in] label The label to set
 * @return 0 if done, -1 on error
 * @pre None
 * @post None
 * @remarks None
 */
    int             vconf_set_label(const char *keyname, const char *label);

/**
 * Set the default group of key lists created using vconf_keylist_new.
 * @param[in] groupname The name of the default group to bind to
 * @return 0 in case of success or -1 in case of error of allocation
 * @see vconf_keylist_new()
 */
    int             vconf_set_default_group(const char *groupname);

/**
 * This function checks if the given key exists from backend system.
 * @param[in]	keyname	key
 * @return 0 on success: exists, -1 on error or not existing
 */
    int             vconf_exists(const char *keyname);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif              /* __VCONF_BUXTON_H__ */
