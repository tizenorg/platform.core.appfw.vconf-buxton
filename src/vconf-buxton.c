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

/*
 * Note on conditionnal compilation: The following defines are controlling the compilation.
 *
 * NO_MULTITHREADING
 *     Defining NO_MULTITHREADING removes multithreading support.
 *     Defining it is not a good idea.
 *
 * NO_GLIB
 *     Defining NO_GLIB removes support of GLIB main loop used for notification
 *     Defining it implies to dig code to find some replacement (new API verbs to create).
 *
 * REMOVE_PREFIXES
 *     Removes the prefixe of the keys depending (depends on the layer).
 *     Defining it is not a good idea.
 *
 * The best is to not define any of the conditional value and let use the default.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <buxton.h>
#if !defined(NO_GLIB)
#include <glib.h>
#endif
#if !defined(NO_MULTITHREADING)
#include <pthread.h>
#endif

#include "vconf-buxton.h"
#include "log.h"

#define VCONF_OK      0
#define VCONF_ERROR  -1

/*================= SECTION definition of types =============*/

/*
 * internal types for keys 
 */
enum keytype
{
  type_unset,			/* type unset or unknown */
  type_directory,		/* a directory, not a real key */
  type_delete,			/* key to be deleted */
  type_string,			/* key of type string */
  type_int,			/* key of type integer */
  type_double,			/* key of type double */
  type_bool			/* key of type boolean */
};

/*
 * union for storing values 
 */
union keyvalue
{
  int i;			/* storage of integers */
  int b;			/* storage of booleans */
  double d;			/* storage of doubles */
  char *s;			/* storage of strings */
};

/*
 * structure for keys 
 */
struct _keynode_t
{
  enum keytype type;		/* type of the key */
  union keyvalue value;		/* value of the key */
  keynode_t *next;		/* linking to the next key */
  keylist_t *list;		/* the containing list */
  const char *keyname;		/* name of the key */
};

/*
 * structure for list of keys 
 */
struct _keylist_t
{
  int num;			/* count of keys in the list */
  keynode_t *head;		/* first key of the list */
  keynode_t *cursor;		/* iterator on keys for clients */
  int cb_active;		/* callback global activity */
  int cb_status;		/* callback global status */
  unsigned cb_sent;		/* callback global count of sent queries */
  unsigned cb_received;		/* callback global count of
				 * received responses */
};

/*
 * data for the callback of scanning 
 */
struct scanning_data
{
  int pending;			/* is the scan pending? */
  int cb_status;		/* status of the call back */
  int want_directories;		/* is scanning directories? */
  int want_keys;		/* is scanning keys? */
  int is_recursive;		/* is scanning recursively? */
  size_t dirlen;		/* length of the directory */
  keylist_t *keylist;		/* keylist to fill */
  const char *prefix;		/* prefix to add in front of names */
  const char *directory;	/* scanned directory */
};

/*
 * data when translating vconf names to buxton names. ** the rule is that
 * name == prefix/key 
 */
struct layer_key
{
  const char *layer;		/* identified layer-name */
  const char *prefix;		/* vconf prefix of the name (without
				 * trailing /) */
  const char *key;		/* buxton key-name (without leading /) */
};

/*
 * singleton data for facilities 
 */
struct singleton
{
  keylist_t list;		/* the list */
  keynode_t node;		/* its single node */
};

/*
 * structure for notifications 
 */
struct notify
{
  int status;			/* callback status */
  vconf_callback_fn callback;	/* the user callback */
  void *userdata;		/* the user data */
  keynode_t *keynode;		/* the recorded key node */
  struct notify *next;		/* tink to the next notification */
};

/*================= SECTION local variables =============*/

/*
 * maximum length of key-names 
 */
static size_t keyname_maximum_length = 2030;

/*
 * maximum length of group-names 
 */
static size_t keygroup_maximum_length = 1010;

/*
 * association from prefixes to layers 
 */
static const char *assoc_prefix_layer[][2] = {
  {"db", "base"},
  {"file", "base"},
  {"memory", "temp"},
  {"memory_init", "base"},
  {"user", "user"},
  {NULL, NULL}
};

/*
 * default timeout in scanning responses 
 */
static int default_timeout = 5000;	/* in milliseconds */

/*
 * instance of the buxton client 
 */
static BuxtonClient the_buxton_client = 0;

/*
 * the file handle number for polling buxton events 
 */
static int the_buxton_client_fd = 0;

/*
 * flag indacating if the buxton client is set or not 
 */
static char the_buxton_client_is_set = 0;

/*
 * the group to use if default group is unset 
 */
static const char initial_default_group[] = "vconf";

/*
 * the default group to use 
 */
static char *default_group = NULL;

/*
 * the notify keylist 
 */
static keylist_t *notify_keylist = NULL;

/*
 * the notify entries 
 */
static struct notify *notify_entries = NULL;

/*
 * the count of lists
 */
static int internal_list_count = 0;

#if !defined(NO_GLIB)
/*
 * link to the glib main loop 
 */
static GSource *glib_source = NULL;
#endif

#if !defined(NO_MULTITHREADING)
/*
 * multithreaded protection
 * CAUTION: always use the given order!
 */
static pthread_mutex_t mutex_notify = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_counter = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_buxton = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_group = PTHREAD_MUTEX_INITIALIZER;
#define LOCK(x) pthread_mutex_lock(&mutex_##x)
#define UNLOCK(x) pthread_mutex_unlock(&mutex_##x)
#else
#define LOCK(x)
#define UNLOCK(x)
#endif

/*================= SECTION utils =============*/

/*
 * duplication of a string with validation of the length 
 */
static inline char *
_dup_ (const char *source, size_t maxlen, const char *tag)
{
  size_t length;
  char *result;

  assert (source != NULL);
  length = strlen (source);
  if (length >= maxlen)
    {
      ERR ("Invalid argument: %s is too long", tag);
      return NULL;
    }
  result = malloc (++length);
  if (result == NULL)
    {
      ERR ("Can't allocate memory for %s", tag);
      return NULL;
    }
  memcpy (result, source, length);
  return result;
}

/*================= SECTION groupname utils =============*/

static inline char *
_dup_groupname_ (const char *groupname)
{
  return _dup_ (groupname, keygroup_maximum_length, "group-name");
}

static inline int
_ensure_default_group_ ()
{
  int result = VCONF_OK;

  LOCK (group);
  if (default_group == NULL)
    {
      default_group = _dup_groupname_ (initial_default_group);
      if (default_group == NULL)
	result = VCONF_ERROR;
    }
  UNLOCK (group);

  return result;
}

/*================= SECTION key utils =============*/

static inline void
_keynode_free_ (keynode_t * keynode)
{
  assert (keynode != NULL);
  if (keynode->type == type_string)
    free (keynode->value.s);
  free (keynode);
}

static inline size_t
_check_keyname_ (const char *keyname)
{
  size_t result;

  assert (keyname != NULL);

  if (*keyname == '/')
    {
      return 0;
    }

  for (result = 0; keyname[result]; result++)
    {
      if (keyname[result] == '/' && keyname[result + 1] == '/')
	{
	  return 0;
	}
    }

  return result;
}

/*================= SECTION list utils =============*/

/*
 * search in 'keylist' an entry of 'keyname' and return it if found or
 * NULL if not found. 'previous' if not NULL and if the entry is found
 * will recieve the address of the pointer referencing the returned node. 
 */
static inline keynode_t *
_keylist_lookup_ (keylist_t * keylist,
		  const char *keyname, keynode_t *** previous)
{
  keynode_t *node, **prev;
  size_t length = 1 + strlen (keyname);

  prev = &keylist->head;
  node = keylist->head;
  while (node)
    {
      assert (node->keyname != NULL);
      if (!memcmp (keyname, node->keyname, length))
	{
	  if (previous)
	    *previous = prev;
	  return node;
	}

      prev = &node->next;
      node = node->next;
    }
  return NULL;
}

static inline keynode_t *
_keylist_add_ (keylist_t * keylist, const char *keyname, enum keytype type)
{
  keynode_t *result;
  char *name;
  size_t length;

  /*
   * not found, create it 
   */
  length = _check_keyname_ (keyname);
  retvm_if (!length, NULL, "invalid keyname");
  retvm_if (length > keyname_maximum_length, NULL, "keyname too long");
  result = malloc (1 + length + sizeof *result);
  retvm_if (result == NULL, NULL, "allocation of keynode failed");
  result->type = type;
  result->value.s = NULL;

  result->list = keylist;
  name = (char *) (result + 1);
  result->keyname = name;
  memcpy (name, keyname, length + 1);

  result->next = keylist->head;
  keylist->head = result;
  keylist->num++;

  return result;
}

static inline keynode_t *
_keylist_getadd_ (keylist_t * keylist, const char *keyname, enum keytype type)
{
  keynode_t *result;

  /*
   * search keynode of keyname 
   */
  result = _keylist_lookup_ (keylist, keyname, NULL);
  if (result == NULL)
    {
      /*
       * not found, create it 
       */
      result = _keylist_add_ (keylist, keyname, type);
    }
  else if (result->type != type)
    {
      if (result->type == type_string)
	free (result->value.s);
      result->type = type;
      result->value.s = NULL;
    }
  return result;
}

static inline int
_keylist_init_singleton_ (struct singleton *singleton, const char *keyname,
			  enum keytype type)
{
  int status;

  if (!_check_keyname_ (keyname))
    {
      ERR ("Invalid key name(%s)", keyname);
      return VCONF_ERROR;
    }
  status = _ensure_default_group_ ();
  if (status != VCONF_OK)
    return status;

  memset (singleton, 0, sizeof *singleton);
  singleton->list.num = 1;
  singleton->list.head = &singleton->node;
  singleton->node.keyname = keyname;
  singleton->node.type = type;
  singleton->node.list = &singleton->list;

  return VCONF_OK;
}


/*================= SECTION buxton =============*/

static void
_check_close_buxton_ ()
{
  BuxtonClient bc;

  LOCK (notify);
  LOCK (counter);
  if (internal_list_count == 0 && notify_entries == NULL)
    if (internal_list_count == 0 && notify_entries == NULL)
      {
	LOCK (buxton);
	bc = the_buxton_client;
	the_buxton_client_is_set = 0;
	the_buxton_client = NULL;
	the_buxton_client_fd = -1;
	UNLOCK (buxton);
	if (bc)
	  buxton_close (bc);
      }
  UNLOCK (counter);
  UNLOCK (notify);
}

static void
_try_to_open_buxton_ ()
{
  the_buxton_client_fd = buxton_open (&the_buxton_client);
  if (the_buxton_client_fd < 0)
    {
      ERR ("can't connect to buxton server: %m");
      errno = ENOTCONN;
      the_buxton_client = NULL;
    }
}

static inline int
_open_buxton_ ()
{
  LOCK (buxton);
  if (!the_buxton_client_is_set)
    {
      /*
       * first time, try to connect to buxton 
       */
      the_buxton_client_is_set = 1;
      _try_to_open_buxton_ ();
    }
  UNLOCK (buxton);
  return the_buxton_client != NULL;
}


static inline BuxtonClient
_buxton_ ()
{
  BuxtonClient result = the_buxton_client;
  assert (result != NULL);
  return result;
}

static inline int
_handle_buxton_response_ (int lock)
{
  int result;

  if (lock)
    LOCK (buxton);
  result = buxton_client_handle_response (_buxton_ ());
  if (result < 0)
    ERR ("Error in buxton_client_handle_response: %m");
  if (result == 0) {
    ERR ("Connection closed");
    result = -1;
  }
  if (lock)
    UNLOCK (buxton);
  return result;
}

static inline int
_dispatch_buxton_ (int writing, int lock)
{
  int status;
  struct pollfd pfd;

  assert (_buxton_ () != NULL);

  pfd.fd = the_buxton_client_fd;
  pfd.events = writing ? POLLIN | POLLOUT : POLLIN;
  for (;;)
    {
      pfd.revents = 0;
      status = poll (&pfd, 1, default_timeout);
      if (status == -1)
	{
	  if (errno != EINTR)
	    {
	      return VCONF_ERROR;
	    }
	}
      else if (status == 1)
	{
	  if (pfd.revents & POLLIN)
	    {
	      status = _handle_buxton_response_ (lock);
	      if (status < 0)
		{
		  return VCONF_ERROR;
		}
	      if (!writing)
		{
		  return VCONF_OK;
		}
	    }
	  if (pfd.revents & POLLOUT)
	    {
	      return VCONF_OK;
	    }
	  else
	    {
	      return VCONF_ERROR;
	    }
	}
    }
}

static inline int
_wait_buxton_response_ (int *pending)
{
  int result;

  do
    {
      result = _dispatch_buxton_ (0, 1);
    }
  while (result == VCONF_OK && *pending);
  return result;
}

static inline int
_get_layer_key_ (const char *keyname, struct layer_key *laykey)
{
  int i, j;
  const char *prefix;

  /*
   * get the names 
   */
  i = 0;
  prefix = assoc_prefix_layer[i][0];
  while (prefix != NULL)
    {
      for (j = 0; prefix[j] && prefix[j] == keyname[j]; j++);
      if (!prefix[j] && (!keyname[j] || keyname[j] == '/'))
	{
	  laykey->prefix = prefix;
	  laykey->layer = assoc_prefix_layer[i][1];
#if defined(REMOVE_PREFIXES)
	  laykey->key = keyname + j + (keyname[j] == '/');
#else
	  laykey->key = keyname;
#endif
	  return VCONF_OK;
	}
      i++;
      prefix = assoc_prefix_layer[i][0];
    }
  ERR ("Invalid argument: wrong prefix of key(%s)", keyname);
  return VCONF_ERROR;
}

static inline BuxtonKey
_get_buxton_key_ (keynode_t * node)
{
  BuxtonDataType type;
  struct layer_key laykey;

  /*
   * get layer and key 
   */
  if (_get_layer_key_ (node->keyname, &laykey) != VCONF_OK)
    {
      return NULL;
    }

  /*
   * get type 
   */
  switch (node->type)
    {
    case type_string:
      type = BUXTON_TYPE_STRING;
      break;
    case type_int:
      type = BUXTON_TYPE_INT32;
      break;
    case type_double:
      type = BUXTON_TYPE_DOUBLE;
      break;
    case type_bool:
      type = BUXTON_TYPE_BOOLEAN;
      break;
    default:
      type = BUXTON_TYPE_UNSET;
    }

  return buxton_key_create (default_group, laykey.key, laykey.layer, type);
}

/*================= SECTION set/unset/refresh =============*/

static void
_cb_inc_received_ (BuxtonResponse resp, keynode_t * keynode)
{
  keylist_t *list;

  assert (keynode != NULL);
  assert (keynode->list != NULL);

  list = keynode->list;
  list->cb_received++;
  if (buxton_response_status (resp) != 0)
    {
      ERR ("Buxton returned error %d for key %s",
	   buxton_response_status (resp), keynode->keyname);
      list->cb_status = VCONF_ERROR;
    }
}

static int
_set_response_to_keynode_ (BuxtonResponse resp, keynode_t * keynode,
			   int force)
{
  enum keytype type;
  BuxtonDataType buxtyp;
  void *buxval;

  assert (keynode != NULL);
  assert (buxton_response_status (resp) == 0);

  buxtyp = buxton_response_value_type (resp);
  switch (buxtyp)
    {
    case BUXTON_TYPE_STRING:
      type = type_string;
      break;
    case BUXTON_TYPE_INT32:
      type = type_int;
      break;
    case BUXTON_TYPE_DOUBLE:
      type = type_double;
      break;
    case BUXTON_TYPE_BOOLEAN:
      type = type_bool;
      break;
    default:
      return VCONF_ERROR;
    }

  if (force && type != keynode->type && keynode->type != type_unset)
    return VCONF_ERROR;

  buxval = buxton_response_value (resp);
  if (buxval == NULL)
    return VCONF_ERROR;

  if (keynode->type == type_string)
    free (keynode->value.s);

  keynode->type = type;
  switch (type)
    {
    case type_string:
      keynode->value.s = buxval;
      return VCONF_OK;
    case type_int:
      keynode->value.i = (int) *(int32_t *) buxval;
      break;
    case type_double:
      keynode->value.d = *(double *) buxval;
      break;
    case type_bool:
      keynode->value.b = *(bool *) buxval;
      break;
    default:
      assert (0);
      break;
    }
  free (buxval);
  return VCONF_OK;
}

static void
_cb_refresh_ (BuxtonResponse resp, keynode_t * keynode)
{
  keylist_t *list;

  assert (keynode != NULL);
  assert (keynode->list != NULL);
  assert (buxton_response_type (resp) == BUXTON_CONTROL_GET);

  list = keynode->list;
  list->cb_received++;
  if (buxton_response_status (resp) != 0)
    {
      ERR ("Error %d while getting buxton key %s",
	   buxton_response_status (resp), keynode->keyname);
      list->cb_status = VCONF_ERROR;
    }
  else if (_set_response_to_keynode_ (resp, keynode, 0) != VCONF_OK)
    {
      list->cb_status = VCONF_ERROR;
    }
}

static void
_cb_scan_ (BuxtonResponse resp, struct scanning_data *data)
{
  char *buxname;
  char *name;
  char *term;
  uint32_t count;
  uint32_t index;
  keylist_t *keylist;
  keynode_t *keynode;
  int length;

  data->pending = 0;

  /*
   * check the response status 
   */
  if (buxton_response_status (resp) != 0)
    {
      ERR ("Error while getting list of names from buxton");
      data->cb_status = VCONF_ERROR;
      return;
    }

  /*
   * iterate on the list of names 
   */
  assert (data->directory[data->dirlen - 1] == '/');
  keylist = data->keylist;
  count = buxton_response_list_names_count (resp);
  index = 0;
  while (index < count)
    {
      /*
       * get the name 
       */
      buxname = buxton_response_list_names_item (resp, index++);
      if (buxname == NULL)
	{
	  ERR ("Unexpected NULL name returned by buxton");
	  data->cb_status = VCONF_ERROR;
	  return;
	}

      /*
       * normalise the name 
       */
#if defined(REMOVE_PREFIXES)
      length = asprintf (&name, "%s/%s", data->prefix, buxname);
#else
      length = asprintf (&name, "%s", buxname);
#endif
      free (buxname);
      if (length < 0)
	{
	  ERR ("Memory allocation error");
	  data->cb_status = VCONF_ERROR;
	  return;
	}
      assert (_check_keyname_ (name));
      assert (!memcmp (data->directory, name, data->dirlen));

      /*
       * add key if requested 
       */
      term = strchr (name + data->dirlen, '/');
      if (data->want_keys && (data->is_recursive || term == NULL))
	{
	  keynode = _keylist_getadd_ (keylist, name, type_unset);
	  if (keynode == NULL)
	    {
	      free (name);
	      data->cb_status = VCONF_ERROR;
	      return;
	    }
	}

      /*
       * add directories if requested 
       */
      if (data->want_directories)
	{
	  while (term != NULL)
	    {
	      *term = 0;
	      keynode = _keylist_getadd_ (keylist, name, type_directory);
	      if (keynode == NULL)
		{
		  free (name);
		  data->cb_status = VCONF_ERROR;
		  return;
		}
	      if (!data->is_recursive)
		{
		  break;
		}
	      *term = '/';
	      term = strchr (term + 1, '/');
	    }
	}

      free (name);
    }
  data->cb_status = VCONF_OK;
}


static inline int
_async_set_ (keynode_t * keynode)
{
  void *data;
  int status;
  BuxtonKey key;

  assert (keynode != NULL);

  switch (keynode->type)
    {
    case type_string:
      data = keynode->value.s;
      break;
    case type_int:
    case type_double:
    case type_bool:
      data = &keynode->value;
      break;
    default:
      return 0;
    }

  key = _get_buxton_key_ (keynode);
  if (key == NULL)
    {
      return -1;
    }

  status = buxton_set_value (_buxton_ (), key,
			     data,
			     (BuxtonCallback) _cb_inc_received_, keynode,
			     false);
  buxton_key_free (key);

  if (status == 0)
    {
      return 1;
    }

  ERR ("Error while calling buxton_set_value: %m");
  return -1;
}

static inline int
_async_unset_ (keynode_t * keynode)
{
  int status;
  BuxtonKey key;

  assert (keynode != NULL);

  if (keynode->type != type_delete)
    {
      return 0;
    }

  key = _get_buxton_key_ (keynode);
  if (key == NULL)
    {
      return -1;
    }

  status = buxton_unset_value (_buxton_ (), key,
			       (BuxtonCallback) _cb_inc_received_,
			       keynode, false);
  buxton_key_free (key);

  if (status == 0)
    {
      return 1;
    }

  ERR ("Error while calling buxton_unset_value: %m");
  return -1;
}

static inline int
_async_set_or_unset_ (keynode_t * keynode, const char *unused)
{
  assert (keynode != NULL);

  switch (keynode->type)
    {
    case type_unset:
    case type_directory:
      return 0;
    case type_delete:
      return _async_unset_ (keynode);
    default:
      return _async_set_ (keynode);
    }
}

static inline int
_async_refresh_ (keynode_t * keynode, const char *unused)
{
  int status;
  BuxtonKey key;

  assert (keynode != NULL);

  switch (keynode->type)
    {
    case type_unset:
    case type_string:
    case type_int:
    case type_double:
    case type_bool:
      break;
    default:
      return 0;
    }

  key = _get_buxton_key_ (keynode);
  if (key == NULL)
    {
      return -1;
    }

  status = buxton_get_value (_buxton_ (), key,
			     (BuxtonCallback) _cb_refresh_, keynode, false);
  buxton_key_free (key);

  if (status == 0)
    {
      return 1;
    }

  ERR ("Error while calling buxton_get_value: %m");
  return -1;
}

static inline int
_async_set_label_ (keynode_t * keynode, const char *label)
{
  int status;
  BuxtonKey key;

  assert (keynode != NULL);

  key = _get_buxton_key_ (keynode);
  if (key == NULL)
    {
      return -1;
    }

  status = buxton_set_label (_buxton_ (), key, label,
			     (BuxtonCallback) _cb_inc_received_,
			     keynode, false);
  buxton_key_free (key);

  if (status == 0)
    {
      return 1;
    }

  ERR ("Error while calling buxton_set_label: %m");
  return -1;
}


static int
_apply_buxton_on_list_ (keylist_t * keylist,
			int (*async) (keynode_t *, const char *),
			const char *data)
{
  keynode_t *keynode;
  int status;
  int sent;

  assert (keylist != NULL);

  status = _open_buxton_ ();
  retvm_if (!status, VCONF_ERROR, "Can't connect to buxton");
  assert (_buxton_ () != NULL);

  retvm_if (keylist->cb_active != 0, VCONF_ERROR,
	    "Already active in vconf-buxton");

  LOCK(buxton);

  keylist->cb_active = 1;
  keylist->cb_status = VCONF_OK;
  keylist->cb_sent = 0;
  keylist->cb_received = 0;

  keynode = keylist->head;
  status = _dispatch_buxton_ (1, 0);
  while (keynode != NULL && status == VCONF_OK)
    {
      sent = async (keynode, data);
      keynode = keynode->next;
      if (sent < 0)
	{
	  status = VCONF_ERROR;
	}
      else if (sent > 0)
	{
	  keylist->cb_sent += sent;
	  status = _dispatch_buxton_ (1, 0);
	}
    }

  /*
   * get the responses 
   */
  while (status == VCONF_OK && keylist->cb_sent != keylist->cb_received)
    {
      status = _dispatch_buxton_ (0, 0);
    }

  if (status == VCONF_OK && keylist->cb_status != VCONF_OK)
    status = keylist->cb_status;
  keylist->cb_active = 0;

  UNLOCK(buxton);

  _check_close_buxton_ ();

  return status;
}

/*================= SECTION notification =============*/

static void
_cb_notify_ (BuxtonResponse resp, struct notify *notif)
{
  switch (buxton_response_type (resp))
    {
    case BUXTON_CONTROL_NOTIFY:
    case BUXTON_CONTROL_UNNOTIFY:
      notif->status =
	buxton_response_status (resp) == 0 ? VCONF_OK : VCONF_ERROR;
      break;
    case BUXTON_CONTROL_CHANGED:
      if (_set_response_to_keynode_ (resp, notif->keynode, 1) == VCONF_OK)
	{
	  UNLOCK (buxton);
	  notif->callback (notif->keynode, notif->userdata);
	  LOCK (buxton);
	}
      break;
    default:
      break;
    }
}

static int
_notify_reg_unreg_ (struct notify *notif, bool reg)
{
  int status;
  BuxtonKey key;

  status = _open_buxton_ ();
  retvm_if (!status, VCONF_ERROR, "Can't connect to buxton");

  LOCK(buxton);
  key = _get_buxton_key_ (notif->keynode);
  retvm_if (key == NULL, VCONF_ERROR, "Can't create buxton key");
  notif->status = VCONF_OK;	/* on success calback isn't called! */
  status =
    (reg ? buxton_register_notification :
     buxton_unregister_notification) (_buxton_ (), key,
				      (BuxtonCallback) _cb_notify_,
				      notif, false);
  buxton_key_free (key);
  UNLOCK(buxton);
  return status == 0 && notif->status == VCONF_OK ? VCONF_OK : VCONF_ERROR;
}

#if !defined(NO_GLIB)
/*================= SECTION glib =============*/

static gboolean
_cb_glib_ (GIOChannel * src, GIOCondition cond, gpointer data)
{
  int status;

  status = _handle_buxton_response_ (1);
  if (status < 0) {
    glib_source = NULL;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

static int
_glib_start_watch_ ()
{
  GIOChannel *gio;

  if (glib_source != NULL)
    return VCONF_OK;

  gio = g_io_channel_unix_new (the_buxton_client_fd);
  retvm_if (gio == NULL, VCONF_ERROR, "Error: create a new GIOChannel");

  g_io_channel_set_flags (gio, G_IO_FLAG_NONBLOCK, NULL);

  glib_source = g_io_create_watch (gio, G_IO_IN);
  if (glib_source == NULL)
    {
      ERR ("Error: create a new GSource");
      g_io_channel_unref (gio);
      return VCONF_ERROR;
    }

  g_source_set_callback (glib_source, (GSourceFunc) _cb_glib_, NULL, NULL);
  g_source_attach (glib_source, NULL);
  g_io_channel_unref (gio);
  g_source_unref (glib_source);

  return VCONF_OK;
}

static void
_glib_stop_watch_ ()
{
  if (glib_source != NULL)
    {
      g_source_destroy (glib_source);
      glib_source = NULL;
    }
}
#endif

/*================= SECTION VCONF API =============*/

const char *
vconf_keynode_get_name (keynode_t * keynode)
{
  retvm_if (keynode == NULL, NULL, "Invalid argument: keynode is NULL");
  retvm_if (keynode->keyname == NULL, NULL, "The name of keynode is NULL");

  return keynode->keyname;
}

int
vconf_keynode_get_type (keynode_t * keynode)
{
  retvm_if (keynode == NULL,
	    VCONF_ERROR, "Invalid argument: keynode is NULL");

  switch (keynode->type)
    {
    case type_directory:
      return VCONF_TYPE_DIR;
    case type_string:
      return VCONF_TYPE_STRING;
    case type_int:
      return VCONF_TYPE_INT;
    case type_double:
      return VCONF_TYPE_DOUBLE;
    case type_bool:
      return VCONF_TYPE_BOOL;
    default:
      return VCONF_TYPE_NONE;
    }
}

int
vconf_keynode_get_int (keynode_t * keynode)
{
  retvm_if (keynode == NULL,
	    VCONF_ERROR, "Invalid argument: keynode is NULL");
  retvm_if (keynode->type != type_int, VCONF_ERROR,
	    "The type of keynode(%s) is not INT", keynode->keyname);

  return keynode->value.i;
}

double
vconf_keynode_get_dbl (keynode_t * keynode)
{
  retvm_if (keynode == NULL, -1.0, "Invalid argument: keynode is NULL");
  retvm_if (keynode->type != type_double, -1.0,
	    "The type of keynode(%s) is not DBL", keynode->keyname);

  return keynode->value.d;
}

int
vconf_keynode_get_bool (keynode_t * keynode)
{
  retvm_if (keynode == NULL,
	    VCONF_ERROR, "Invalid argument: keynode is NULL");
  retvm_if (keynode->type != type_bool, VCONF_ERROR,
	    "The type of keynode(%s) is not BOOL", keynode->keyname);

  return !!(keynode->value.b);
}

char *
vconf_keynode_get_str (keynode_t * keynode)
{
  retvm_if (keynode == NULL, NULL, "Invalid argument: keynode is NULL");
  retvm_if (keynode->type != type_string, NULL,
	    "The type of keynode(%s) is not STR", keynode->keyname);

  return keynode->value.s;
}

int
vconf_set_default_group (const char *groupname)
{
  char *copy;

  copy = _dup_groupname_ (groupname);
  if (copy == NULL)
    return VCONF_ERROR;
  free (default_group);
  default_group = copy;
  return VCONF_OK;
}

keylist_t *
vconf_keylist_new ()
{
  keylist_t *result;
  if (_ensure_default_group_ () != VCONF_OK)
    return NULL;
  result = calloc (1, sizeof (keylist_t));
  LOCK (counter);
  internal_list_count += (result != NULL);
  UNLOCK (counter);
  return result;
}

int
vconf_keylist_free (keylist_t * keylist)
{
  keynode_t *keynode, *temp;

  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");

  keynode = keylist->head;
  free (keylist);
  while (keynode)
    {
      temp = keynode->next;
      _keynode_free_ (keynode);
      keynode = temp;
    }

  LOCK (counter);
  internal_list_count -= (internal_list_count > 0);
  UNLOCK (counter);
  _check_close_buxton_ ();
  return 0;
}

int
vconf_keylist_rewind (keylist_t * keylist)
{
  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");

  keylist->cursor = NULL;

  return 0;
}

keynode_t *
vconf_keylist_nextnode (keylist_t * keylist)
{
  keynode_t *result;

  retvm_if (keylist == NULL, NULL, "Invalid argument: keylist is NULL");

  result = keylist->cursor;
  result = result == NULL ? keylist->head : result->next;
  keylist->cursor = result;

  return result;
}

int
vconf_keylist_lookup (keylist_t * keylist,
		      const char *keyname, keynode_t ** return_node)
{
  keynode_t *keynode;

  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL,
	    VCONF_ERROR, "Invalid argument: keyname is NULL");
  retvm_if (return_node == NULL,
	    VCONF_ERROR, "Invalid argument: return_node is NULL");

  keynode = _keylist_lookup_ (keylist, keyname, NULL);
  if (NULL == keynode)
    return 0;

  if (return_node)
    *return_node = keynode;
  return keynode->type;
}

static int
_cb_sort_keynodes (const void *a, const void *b)
{
  register const keynode_t *kna = *(const keynode_t **) a;
  register const keynode_t *knb = *(const keynode_t **) b;
  return strcmp (kna->keyname, knb->keyname);
}

int
vconf_keylist_sort (keylist_t * keylist)
{
  int index;
  keynode_t **nodes, *keynode;

  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");

  if (keylist->num <= 1)
    return VCONF_OK;

  nodes = malloc (keylist->num * sizeof *nodes);
  retvm_if (nodes == NULL, VCONF_ERROR, "can't allocate memory for sorting");

  index = 0;
  keynode = keylist->head;
  while (keynode != NULL)
    {
      assert (index < keylist->num);
      nodes[index++] = keynode;
      keynode = keynode->next;
    }
  assert (index == keylist->num);

  qsort (nodes, index, sizeof *nodes, _cb_sort_keynodes);

  while (index)
    {
      nodes[--index]->next = keynode;
      keynode = nodes[index];
    }
  keylist->head = keynode;
  free (nodes);
  return VCONF_OK;
}

int
vconf_keylist_add_int (keylist_t * keylist, const char *keyname,
		       const int value)
{
  keynode_t *keynode;

  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL,
	    VCONF_ERROR, "Invalid argument: keyname is NULL");

  keynode = _keylist_getadd_ (keylist, keyname, type_int);
  if (keynode == NULL)
    return VCONF_ERROR;

  keynode->value.i = value;
  return keylist->num;
}

int
vconf_keylist_add_bool (keylist_t * keylist, const char *keyname,
			const int value)
{
  keynode_t *keynode;

  retvm_if (keylist == NULL,
	    VCONF_ERROR, "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL,
	    VCONF_ERROR, "Invalid argument: keyname is NULL");

  keynode = _keylist_getadd_ (keylist, keyname, type_bool);
  if (keynode == NULL)
    return VCONF_ERROR;

  keynode->value.b = !!value;
  return keylist->num;
}

int
vconf_keylist_add_dbl (keylist_t * keylist,
		       const char *keyname, const double value)
{
  keynode_t *keynode;

  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL, VCONF_ERROR,
	    "Invalid argument: keyname is NULL");

  keynode = _keylist_getadd_ (keylist, keyname, type_double);
  if (keynode == NULL)
    return VCONF_ERROR;

  keynode->value.d = value;
  return keylist->num;
}

int
vconf_keylist_add_str (keylist_t * keylist,
		       const char *keyname, const char *value)
{
  keynode_t *keynode;
  char *copy;

  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL, VCONF_ERROR,
	    "Invalid argument: keyname is NULL");

  copy = strdup (value == NULL ? "" : value);
  retvm_if (copy == NULL, VCONF_ERROR, "Allocation of memory failed");

  keynode = _keylist_getadd_ (keylist, keyname, type_string);
  if (keynode == NULL)
    {
      free (copy);
      return VCONF_ERROR;
    }

  free (keynode->value.s);
  keynode->value.s = copy;
  return keylist->num;
}

int
vconf_keylist_add_null (keylist_t * keylist, const char *keyname)
{
  keynode_t *keynode;

  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL, VCONF_ERROR,
	    "Invalid argument: keyname is NULL");

  keynode = _keylist_getadd_ (keylist, keyname, type_unset);
  if (keynode == NULL)
    return VCONF_ERROR;

  return keylist->num;
}

int
vconf_keylist_del (keylist_t * keylist, const char *keyname)
{
  keynode_t *keynode, **previous = NULL;

  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");
  retvm_if (keyname == NULL, VCONF_ERROR,
	    "Invalid argument: keyname is NULL");

  keynode = _keylist_lookup_ (keylist, keyname, &previous);
  if (keynode == NULL)
    return VCONF_ERROR;

  *previous = keynode->next;
  keylist->num--;
  _keynode_free_ (keynode);

  return VCONF_OK;
}

int
vconf_set (keylist_t * keylist)
{
  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");

  return _apply_buxton_on_list_ (keylist, _async_set_or_unset_, NULL);
}

int
vconf_set_labels (keylist_t * keylist, const char *label)
{
  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");

  retvm_if (label == NULL, VCONF_ERROR, "Invalid argument: name is NULL");

  return _apply_buxton_on_list_ (keylist, _async_set_label_, label);
}

int
vconf_sync_key (const char *keyname)
{
  /*
   * does nothing succefully 
   */
  return 0;
}

int
vconf_refresh (keylist_t * keylist)
{
  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is NULL");

  return _apply_buxton_on_list_ (keylist, _async_refresh_, NULL);
}

int
vconf_scan (keylist_t * keylist, const char *dirpath, get_option_t option)
{
  char *dircopy;
  struct layer_key laykey;
  struct scanning_data data;
  int status;

  retvm_if (keylist == NULL, VCONF_ERROR,
	    "Invalid argument: keylist is null");
  retvm_if (keylist->num != 0, VCONF_ERROR,
	    "Invalid argument: keylist not empty");
  retvm_if (dirpath == NULL, VCONF_ERROR,
	    "Invalid argument: dirpath is null");
  retvm_if (_check_keyname_ (dirpath) == 0, VCONF_ERROR,
	    "Invalid argument: dirpath is not valid");

  status = _open_buxton_ ();
  if (!status)
    {
      ERR ("Can't connect to buxton");
      return VCONF_ERROR;
    }

  data.keylist = keylist;

  switch (option)
    {
    case VCONF_GET_KEY:
      data.want_directories = 0;
      data.want_keys = 1;
      data.is_recursive = 0;
      break;
    case VCONF_GET_ALL:
      data.want_directories = 1;
      data.want_keys = 1;
      data.is_recursive = 0;
      break;
    case VCONF_GET_DIR:
      data.want_directories = 1;
      data.want_keys = 0;
      data.is_recursive = 0;
      break;
    case VCONF_GET_KEY_REC:
      data.want_directories = 0;
      data.want_keys = 1;
      data.is_recursive = 1;
      break;
    case VCONF_GET_ALL_REC:
      data.want_directories = 1;
      data.want_keys = 1;
      data.is_recursive = 1;
      break;
    case VCONF_GET_DIR_REC:
      data.want_directories = 0;
      data.want_keys = 1;
      data.is_recursive = 1;
      break;
    default:
      ERR ("Invalid argument: Bad option value");
      return VCONF_ERROR;
    }

  data.dirlen = strlen (dirpath);
  assert (data.dirlen);
  if (dirpath[data.dirlen - 1] == '/')
    {
      data.directory = dirpath;
      dircopy = NULL;
    }
  else
    {
      status = asprintf (&dircopy, "%s/", dirpath);
      retvm_if (status < 0, VCONF_ERROR,
		"No more memory for copying dirpath");
      data.directory = dircopy;
      data.dirlen++;
    }

  status = _get_layer_key_ (data.directory, &laykey);
  if (status != VCONF_OK)
    {
      return status;
    }

  data.prefix = laykey.prefix;
  if (!laykey.key[0])
    {
      laykey.key = NULL;
    }

  data.pending = 1;
  assert (_buxton_ () != NULL);
  status = buxton_list_names (_buxton_ (), laykey.layer, default_group,
			      laykey.key, (BuxtonCallback) _cb_scan_,
			      &data, false);
  if (!status)
    status = _wait_buxton_response_ (&data.pending);

  free (dircopy);

  retvm_if (status, VCONF_ERROR, "Error while calling buxton_list_names: %m");
  if (data.cb_status != VCONF_OK)
    {
      return VCONF_ERROR;
    }

  return vconf_refresh (keylist);
}

int
vconf_get (keylist_t * keylist, const char *dirpath, get_option_t option)
{
  if (option == VCONF_REFRESH_ONLY
      || (option == VCONF_GET_KEY && keylist->num != 0))
    {
      return vconf_refresh (keylist);
    }
  else
    {
      return vconf_scan (keylist, dirpath, option);
    }
}

int
vconf_unset (const char *keyname)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_delete);
  if (status == VCONF_OK)
    {
      status = vconf_set (&single.list);
    }
  return status;

}

int
vconf_exists (const char *keyname)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_unset);
  if (status == VCONF_OK)
    {
      status = vconf_refresh (&single.list);
      if (status == VCONF_OK && single.node.type == type_string)
	free (single.node.value.s);
    }
  return status;

}

int
vconf_unset_recursive (const char *in_dir)
{
  struct _keylist_t *keylist;
  struct _keynode_t *keynode;
  int status;

  retvm_if (in_dir == NULL, VCONF_ERROR, "Invalid argument: dir is null");

  keylist = vconf_keylist_new ();
  if (keylist == NULL)
    return VCONF_ERROR;

  status = vconf_scan (keylist, in_dir, VCONF_GET_KEY_REC);
  if (status == VCONF_OK)
    {
      for (keynode = keylist->head; keynode; keynode = keynode->next)
	keynode->type = type_delete;
      status = vconf_set (keylist);
    }
  vconf_keylist_free (keylist);
  return status;
}

int
vconf_notify_key_changed (const char *keyname, vconf_callback_fn cb,
			  void *user_data)
{
  int status;
  struct notify *notif;
  keynode_t *keynode, *aknode;
  keylist_t *alist;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is null");
  retvm_if (cb == NULL, VCONF_ERROR, "Invalid argument: cb(%p)", cb);
  status = _open_buxton_ ();
  retvm_if (!status, VCONF_ERROR, "Can't connect to buxton");
  status = vconf_exists (keyname);
  retvm_if (status != VCONF_OK, VCONF_ERROR, "key %s doesn't exist", keyname);


  /*
   * create the notification 
   */
  notif = malloc (sizeof *notif);
  retvm_if (notif == NULL, VCONF_ERROR,
	    "Allocation of notify structure failed");

  /*
   * ensure existing list 
   */
  LOCK (notify);
  if (notify_keylist == NULL)
    {
      notify_keylist = vconf_keylist_new ();
      if (notify_keylist == NULL)
	{
	  UNLOCK (notify);
	  free (notif);
	  return VCONF_ERROR;
	}
    }

  /*
   * search keynode of keyname 
   */
  keynode = _keylist_lookup_ (notify_keylist, keyname, NULL);
  if (keynode == NULL)
    {
      /*
       * not found, create it with type unset 
       */
      keynode = _keylist_add_ (notify_keylist, keyname, type_unset);
      if (keynode == NULL)
	{
	  UNLOCK (notify);
	  free (notif);
	  return VCONF_ERROR;
	}
    }

  /*
   * init the notification 
   */
  notif->callback = cb;
  notif->userdata = user_data;
  notif->keynode = keynode;
  notif->next = notify_entries;
  notify_entries = notif;
  UNLOCK (notify);

  /*
   * record the notification 
   */
  status = _notify_reg_unreg_ (notif, true);
  if (status != VCONF_OK)
    {
      vconf_ignore_key_changed (keyname, cb);
      return VCONF_ERROR;
    }

#if !defined(NO_GLIB)
  return _glib_start_watch_ ();
#else
  return VCONF_OK;
#endif
}

int
vconf_ignore_key_changed (const char *keyname, vconf_callback_fn cb)
{
  struct notify *entry, **prevent, *delent, **prevdelent;
  keynode_t *keynode, **prevnod;
  int fcount;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is null");
  retvm_if (cb == NULL, VCONF_ERROR, "Invalid argument: cb(%p)", cb);
  status = _open_buxton_ ();
  retvm_if (!status, VCONF_ERROR, "Can't connect to buxton");

  fcount = 0;
  status = VCONF_ERROR;
  delent = NULL;

  LOCK (notify);
  if (notify_keylist != NULL)
    {
      keynode = _keylist_lookup_ (notify_keylist, keyname, &prevnod);
      if (keynode != NULL)
	{
	  prevdelent = &delent;
	  prevent = &notify_entries;
	  entry = notify_entries;
	  while (entry != NULL)
	    {
	      if (entry->keynode == keynode)
		{
		  if (entry->callback == cb)
		    {
		      *prevdelent = entry;
		      prevdelent = &entry->next;
		      entry = entry->next;
		      continue;
		    }
		  fcount++;
		}
	      *prevent = entry;
	      prevent = &entry->next;
	      entry = entry->next;
	    }
	  *prevent = NULL;
	  *prevdelent = NULL;
	  if (fcount == 0)
	    *prevnod = keynode->next;
#if !defined(NO_GLIB)
	  if (notify_entries == NULL)
	    _glib_stop_watch_ ();
#endif
	  if (delent != NULL)
	    {
	      UNLOCK (notify);
	      while (delent != NULL)
		{
		  entry = delent;
		  delent = entry->next;
		  _notify_reg_unreg_ (entry, false);
		  free (entry);
		}
	      if (fcount == 0)
		_keynode_free_ (keynode);
	      return VCONF_OK;
	    }
	}
    }
  UNLOCK (notify);
  ERR ("Not found: can't remove notification for key(%s)", keyname);

  return VCONF_ERROR;
}

int
vconf_set_int (const char *keyname, const int intval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_int);
  if (status == VCONF_OK)
    {
      single.node.value.i = intval;
      status = vconf_set (&single.list);
    }
  return status;
}

int
vconf_set_bool (const char *keyname, const int boolval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_bool);
  if (status == VCONF_OK)
    {
      single.node.value.b = (bool) boolval;
      status = vconf_set (&single.list);
    }
  return status;
}

int
vconf_set_dbl (const char *keyname, const double dblval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_double);
  if (status == VCONF_OK)
    {
      single.node.value.d = dblval;
      status = vconf_set (&single.list);
    }
  return status;
}

int
vconf_set_str (const char *keyname, const char *strval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_string);
  if (status == VCONF_OK)
    {
      single.node.value.s = (char *) strval;
      status = vconf_set (&single.list);
    }
  return status;
}

int
vconf_get_int (const char *keyname, int *intval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_int);
  if (status == VCONF_OK)
    {
      status = vconf_refresh (&single.list);
      if (status == VCONF_OK)
	*intval = single.node.value.i;
    }
  return status;
}

int
vconf_set_label (const char *keyname, const char *label)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  retvm_if (label == NULL, VCONF_ERROR, "Invalid argument: name is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_unset);
  if (status == VCONF_OK)
    {
      status = vconf_set_labels (&single.list, label);
    }
  return status;
}

int
vconf_get_bool (const char *keyname, int *boolval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_bool);
  if (status == VCONF_OK)
    {
      status = vconf_refresh (&single.list);
      if (status == VCONF_OK)
	*boolval = (int) single.node.value.b;
    }
  return status;
}

int
vconf_get_dbl (const char *keyname, double *dblval)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_double);
  if (status == VCONF_OK)
    {
      status = vconf_refresh (&single.list);
      if (status == VCONF_OK)
	*dblval = single.node.value.d;
    }
  return status;
}

char *
vconf_get_str (const char *keyname)
{
  struct singleton single;
  int status;

  retvm_if (keyname == NULL, NULL, "Invalid argument: key is NULL");

  status = _keylist_init_singleton_ (&single, keyname, type_string);
  if (status != VCONF_OK)
    return NULL;

  single.node.value.s = NULL;
  status = vconf_refresh (&single.list);
  if (status != VCONF_OK)
    return NULL;

  return single.node.value.s;
}
