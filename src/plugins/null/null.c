/***************************************************************************
                     null.c  -  Skeleton of a plugin
                             -------------------
    begin                : Fri May 21 2010
    copyright            : (C) 2010 by Markus Raab
    email                : elektra@markus-raab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This is the skeleton of the methods you'll have to implement in order *
 *   to provide a valid plugin.                                            *
 *   Simple fill the empty functions with your code and you are            *
 *   ready to go.                                                          *
 *                                                                         *
 ***************************************************************************/


#include "null.h"

#include <string.h>
#include <stdlib.h>

int elektraNullGet(Plugin *handle, KeySet *returned, Key *parentKey)
{
	if (!strcmp (keyName(parentKey), "system/elektra/modules/null"))
	{
		KeySet *moduleConfig = ksNew (30,
			keyNew ("system/elektra/modules/null",
				KEY_VALUE, "null plugin waits for your orders", KEY_END),
			keyNew ("system/elektra/modules/null/exports", KEY_END),
			keyNew ("system/elektra/modules/null/exports/get",
				KEY_FUNC, elektraNullGet, KEY_END),
			keyNew ("system/elektra/modules/null/exports/set",
				KEY_FUNC, elektraNullSet, KEY_END),
			keyNew ("system/elektra/modules/null/infos",
				KEY_VALUE, "All information you want to know", KEY_END),
			keyNew ("system/elektra/modules/null/infos/author",
				KEY_VALUE, "Markus Raab <elektra@markus-raab.org>", KEY_END),
			keyNew ("system/elektra/modules/null/infos/licence",
				KEY_VALUE, "BSD", KEY_END),
			keyNew ("system/elektra/modules/null/infos/description",
				KEY_VALUE, "Avoids null and empty values", KEY_END),
			keyNew ("system/elektra/modules/null/infos/provides",
				KEY_VALUE, "", KEY_END),
			keyNew ("system/elektra/modules/null/infos/placements",
				KEY_VALUE, "postgetstorage presetstorage", KEY_END),
			keyNew ("system/elektra/modules/null/infos/needs",
				KEY_VALUE, "", KEY_END),
			keyNew ("system/elektra/modules/null/infos/version",
				KEY_VALUE, PLUGINVERSION, KEY_END),
			KS_END);
		ksAppend (returned, moduleConfig);
		ksDel (moduleConfig);

		return 1;
	}
	/* get all keys */

	Key *k;
	ksRewind (returned);
	while ((k = ksNext (returned)) != 0)
	{
		if (!strcmp (keyString(k), "@NULL"))
		{
			keySetBinary (k, 0, 0);
		}
		else if (!strcmp (keyString(k), "@EMPTY"))
		{
			keySetString (k, "");
		}
		else if (!strncmp (keyString(k), "@@", 2))
		{
			/* Drop the first of the @ */
			keySetString(k, keyString(k)+1);
		}
	}

	return 1; /* success */
}

int elektraNullSet(Plugin *handle, KeySet *returned, Key *parentKey)
{
	/* set all keys */

	Key *k;
	ksRewind (returned);
	while ((k = ksNext (returned)) != 0)
	{
		if (keyValue(k) == 0)
		{
			keySetString (k, "@NULL");
		}
		else if (!strcmp (keyValue(k), ""))
		{
			keySetString (k, "@EMPTY");
		}
		else if (!strncmp (keyValue(k), "@", 1))
		{
			char *n = malloc (keyGetValueSize(k)+1);
			strcpy (n, "@");
			strcat (n, keyValue(k));
			keySetString (k, n);
			free (n);
		}
	}

	return 1; /* success */
}

Plugin *ELEKTRA_PLUGIN_EXPORT(null)
{
	return elektraPluginExport("null",
		ELEKTRA_PLUGIN_GET,	&elektraNullGet,
		ELEKTRA_PLUGIN_SET,	&elektraNullSet,
		ELEKTRA_PLUGIN_END);
}
