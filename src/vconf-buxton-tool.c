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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vconf-buxton.h"
#include "log.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <wordexp.h>
#include <glib.h>

enum
{
  VCONFTOOL_TYPE_NO = 0x00,
  VCONFTOOL_TYPE_STRING,
  VCONFTOOL_TYPE_INT,
  VCONFTOOL_TYPE_DOUBLE,
  VCONFTOOL_TYPE_BOOL
};

static char *guid = NULL;
static char *uid = NULL;
static char *vconf_type = NULL;
static int is_recursive = FALSE;
static int is_initialization = FALSE;
static int is_forced = FALSE;
static int get_num = 0;

static GOptionEntry entries[] = {
  {"type", 't', 0, G_OPTION_ARG_STRING, &vconf_type, "type of value",
   "int|bool|double|string"},
  {"recursive", 'r', 0, G_OPTION_ARG_NONE, &is_recursive,
   "retrieve keys recursively", NULL},
  {"guid", 'g', 0, G_OPTION_ARG_STRING, &guid, "group permission", NULL},
  {"uid", 'u', 0, G_OPTION_ARG_STRING, &uid, "user permission", NULL},
  {"initialization", 'i', 0, G_OPTION_ARG_NONE, &is_initialization,
   "memory backend initialization", NULL},
  {"force", 'f', 0, G_OPTION_ARG_NONE, &is_forced,
   "overwrite vconf values by force", NULL},
  {NULL}
};

static void get_operation (const char *input);
static void print_keylist (keylist_t * keylist);

static char usage[] =
  "Usage:\n"
  "\n"
  "[Set vconf value]\n"
  "       %1$s set -t <TYPE> <KEY NAME> <VALUE> <OPTIONS>\n"
  "                 <TYPE>=int|bool|double|string\n"
  "\n"
  "       Ex) %1$s set -t string db/testapp/key1 \"This is test\" \n"
  "\n"
  "       <OPTIONS>\n"
  "          any option is ignored! (compatibility)\n"
  "\n"
  "[Get vconf value]\n"
  "       %1$s get <OPTIONS> <KEY NAME>\n"
  "\n"
  "       <OPTIONS>\n"
  "          -r : retrieve all keys included in sub-directorys \n"
  "       Ex) %1$s get db/testapp/key1\n"
  "           %1$s get db/testapp/\n"
  "\n"
  "[Unset vconf value]\n"
  "       %1$s unset <KEY NAME>\n"
  "\n" "       Ex) %1$s unset db/testapp/key1\n" "\n"
  "\n"
  "[Set vconf label (Smack)]\n"
  "       %1$s label <KEY NAME> <SMACK LABEL>\n"
  "\n" "       Ex) %1$s label db/testapp/key1 User::Share\n" "\n";

static void
print_help (const char *cmd)
{
  fprintf (stderr, usage, cmd);
}

static int
check_type (void)
{
  if (vconf_type)
    {
      if (!strncasecmp (vconf_type, "int", 3))
	return VCONFTOOL_TYPE_INT;
      else if (!strncasecmp (vconf_type, "string", 6))
	return VCONFTOOL_TYPE_STRING;
      else if (!strncasecmp (vconf_type, "double", 6))
	return VCONFTOOL_TYPE_DOUBLE;
      else if (!strncasecmp (vconf_type, "bool", 4))
	return VCONFTOOL_TYPE_BOOL;
    }
  return VCONFTOOL_TYPE_NO;
}

int
main (int argc, char **argv)
{
  int set_type;

  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("- vconf library tool");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_ignore_unknown_options (context, TRUE);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

  if (argc < 2)
    {
      print_help (argv[0]);
      return 1;
    }

  if (!strcmp (argv[1], "set"))
    {
      set_type = check_type ();
      if (argc < 4 || !set_type)
	{
	  print_help (argv[0]);
	  return 1;
	}

      switch (set_type)
	{
	case VCONFTOOL_TYPE_STRING:
	  vconf_set_str (argv[2], argv[3]);
	  break;
	case VCONFTOOL_TYPE_INT:
	  vconf_set_int (argv[2], atoi (argv[3]));
	  break;
	case VCONFTOOL_TYPE_DOUBLE:
	  vconf_set_dbl (argv[2], atof (argv[3]));
	  break;
	case VCONFTOOL_TYPE_BOOL:
	  vconf_set_bool (argv[2], !!atoi (argv[3]));
	  break;
	default:
	  fprintf (stderr, "never reach");
	  exit (1);
	}

    }
  else if (!strcmp (argv[1], "get"))
    {
      if (argv[2])
	get_operation (argv[2]);
      else
	print_help (argv[0]);
    }
  else if (!strcmp (argv[1], "unset"))
    {
      if (argv[2])
	vconf_unset (argv[2]);
      else
	print_help (argv[0]);
    }
  else if (!strcmp (argv[1], "label"))
    {
      if (argv[2] && argv[3])
	vconf_set_label (argv[2], argv[3]);
      else
	print_help (argv[0]);
    }
  else
    fprintf (stderr, "%s is a invalid command\n", argv[1]);
  return 0;
}

static void
get_operation (const char *input)
{
  keylist_t *keylist;

  keylist = vconf_keylist_new ();
  if (is_recursive)
    {
      vconf_scan (keylist, input, VCONF_GET_KEY_REC);
    }
  else
    {
      vconf_keylist_add_null (keylist, input);
      vconf_refresh (keylist);
    }
  vconf_keylist_sort (keylist);
  print_keylist (keylist);
  if (!get_num)
    printf ("No data\n");
  vconf_keylist_free (keylist);
}

static void
print_keylist (keylist_t * keylist)
{
  keynode_t *node;

  vconf_keylist_rewind (keylist);
  while ((node = vconf_keylist_nextnode (keylist)))
    {
      switch (vconf_keynode_get_type (node))
	{
	case VCONF_TYPE_INT:
	  printf ("%s, value = %d (int)\n",
		  vconf_keynode_get_name (node),
		  vconf_keynode_get_int (node));
	  get_num++;
	  break;
	case VCONF_TYPE_BOOL:
	  printf ("%s, value = %d (bool)\n",
		  vconf_keynode_get_name (node),
		  vconf_keynode_get_bool (node));
	  get_num++;
	  break;
	case VCONF_TYPE_DOUBLE:
	  printf ("%s, value = %f (double)\n",
		  vconf_keynode_get_name (node),
		  vconf_keynode_get_dbl (node));
	  get_num++;
	  break;
	case VCONF_TYPE_STRING:
	  printf ("%s, value = %s (string)\n",
		  vconf_keynode_get_name (node),
		  vconf_keynode_get_str (node));
	  get_num++;
	  break;
	default:
	  break;
	}
    }
}
