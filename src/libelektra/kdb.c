/***************************************************************************
            kdb.c  -  Low level functions for access the Key Database
                             -------------------
 *  begin                : Wed 19 May, 2010
 *  copyright            : (C) 2010 by Markus Raab
 *  email                : elektra@markus-raab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/


/**
 * @defgroup kdb KDB
 * @brief General methods to access the Key database.
 *
 * To use them:
 * @code
 * #include <kdb.h>
 * @endcode
 *
 * The kdb*() methods are used to access the storage, to get and set
 * @link keyset KeySets@endlink.
 *
 * Parameters common for all these functions are:
 *
 * - *handle*, as returned by kdbOpen(), need to be passed to every call
 * - *parentKey* is used for every call to add warnings and set an
 *   error. For kdbGet() / kdbSet() it is used to give an hint which keys
 *   should be retrieved/stored.
 *
 * @note The parentKey is an obligation for you, but only an hint for KDB.
 * KDB does not remember anything
 * about the configuration. You need to pass the same configuration
 * back to kdbSet(), otherwise parts of the configuration get
 * lost. Only keys below the parentKey are subject for change, the rest
 * must be left untouched.
 *
 * KDB uses different backend implementations that know the details
 * about how to access the storage.
 * One backend consists of multiple plugins.
 * See @link plugin writing a new plugin @endlink for information
 * about how to write a plugin.
 * Backends are state-less regarding the configuration (because of that
 * you must pass back the whole configuration for every backend), but
 * have a state for:
 *
 * - a two phase-commit
 * - a conflict detection (error 30) and
 * - optimizations that avoid redoing already done operations.
 *
 * @image html state.png "State"
 * @image latex state.png "State"
 *
 * As we see in the figure, kdbOpen() can be called arbitrarily often in any
 * number of threads.
 *
 * For every handle you got from kdbOpen(), for every parentKey with a
 * different name, *only* the shown state transitions
 * are valid. From a freshly opened KDB, only kdbGet() and kdbClose()
 * are allowed, because otherwise conflicts (error 30) would not be detected.
 *
 * Once kdbGet() was called (for a specific handle+parentKey),
 * any number of kdbGet() and kdbSet() can be
 * used with this handle respective parentKey, unless kdbSet() had
 * a conflict (error 30) with another application.
 * Every affair with KDB needs to be finished with kdbClose().
 *
 * The name of the parentKey in kdbOpen() and kdbClose() does not matter.
 *
 * In the usual case we just have one parentKey and one handle. In
 * these cases we just have to remember to use kdbGet() before
 * kdbSet():
 *
 * @include kdbintro.c
 *
 * To output warnings, you can use following code:
 *
 * @snippet tests.c warnings
 *
 * To output the error, you can use following code:
 *
 * @snippet tests.c error
 *
 * @{
 */


#ifdef HAVE_KDBCONFIG_H
#include "kdbconfig.h"
#endif

#if DEBUG && defined(HAVE_STDIO_H)
#include <stdio.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <kdbinternal.h>


/**
 * @brief Opens the session with the Key database.
 *
 * @pre errorKey must be a valid key, e.g. created with keyNew()
 *
 * The method will bootstrap itself the following way.
 * The first step is to open the default backend. With it
 * system/elektra/mountpoints will be loaded and all needed
 * libraries and mountpoints will be determined.
 * These libraries for backends will be loaded and with it the
 * @p KDB datastructure will be initialized.
 *
 * You must always call this method before retrieving or committing any
 * keys to the database. In the end of the program,
 * after using the key database, you must not forget to kdbClose().
 *
 * The pointer to the @p KDB structure returned will be initialized
 * like described above, and it must be passed along on any kdb*()
 * method your application calls.
 *
 * Get a @p KDB handle for every thread using elektra. Don't share the
 * handle across threads, and also not the pointer accessing it:
 *
 * @snippet kdbopen.c open
 *
 * You don't need kdbOpen() if you only want to
 * manipulate plain in-memory Key or KeySet objects.
 *
 * @pre errorKey must be a valid key, e.g. created with keyNew()
 *
 * @param errorKey the key which holds errors and warnings which were issued
 * @see kdbGet(), kdbClose() to end all affairs to the key database.
 * @retval handle on success
 * @retval NULL on failure
 * @ingroup kdb
 */
KDB * kdbOpen(Key *errorKey)
{
	KDB *handle;
	KeySet *keys;

	if (!errorKey)
	{
		return 0;
	}

	int errnosave = errno;
	handle = elektraCalloc(sizeof(struct _KDB));

	handle->modules = ksNew(0, KS_END);
	if(elektraModulesInit(handle->modules, errorKey) == -1)
	{
		ksDel(handle->modules);
		elektraFree(handle);
		ELEKTRA_SET_ERROR(94, errorKey,
				"elektraModulesInit returned with -1");
		errno = errnosave;
		return 0;
	}

	handle->defaultBackend=elektraBackendOpenDefault(handle->modules,
			errorKey);
	if(!handle->defaultBackend)
	{
		ksDel(handle->modules);
		elektraFree(handle);
		ELEKTRA_SET_ERROR(40, errorKey,
				"could not open default backend");
		errno = errnosave;
		return 0;
	}

	handle->split = elektraSplitNew();
	elektraSplitAppend (handle->split, handle->defaultBackend,
			keyNew (KDB_KEY_MOUNTPOINTS, KEY_END), 2);

	keys=ksNew(0, KS_END);

	Key *initialParent = keyDup (errorKey);
	keySetName(errorKey, KDB_KEY_MOUNTPOINTS);

	if (kdbGet(handle, keys, errorKey) == -1)
	{
		ELEKTRA_ADD_WARNING(17, errorKey,
				"kdbGet() of " KDB_KEY_MOUNTPOINTS
				" failed");
		elektraBackendClose(handle->defaultBackend, errorKey);
		elektraSplitDel(handle->split);
		handle->defaultBackend = 0;
		handle->trie = 0;

		keySetName(errorKey, keyName(initialParent));
		keyDel(initialParent);
		errno = errnosave;
		return handle;
	}
	keySetName(errorKey, keyName(initialParent));

	elektraBackendClose(handle->defaultBackend, errorKey);
	elektraSplitDel(handle->split);
	handle->defaultBackend = 0;
	handle->trie = 0;

#if DEBUG && VERBOSE
	Key *key;

	ksRewind(keys);
	for (key=ksNext(keys);key;key=ksNext(keys))
	{
		printf("config for createTrie name: %s value: %s\n",
				keyName(key), keyString(key));
	}
#endif

	handle->split = elektraSplitNew();

	// Open the trie, keys will be deleted within elektraMountOpen
	if (elektraMountOpen(handle, keys, handle->modules, errorKey) == -1)
	{
		ELEKTRA_ADD_WARNING(93, errorKey,
				"Initial loading of trie did not work");
	}

	if (elektraMountDefault(handle, handle->modules, errorKey) == -1)
	{
		ELEKTRA_SET_ERROR(40, errorKey,
				"could not reopen and mount default backend");
		kdbClose(handle, errorKey);
		keyDel (initialParent);
		errno = errnosave;
		return 0;
	}

	if (elektraMountModules(handle, handle->modules, errorKey) == -1)
	{
		ELEKTRA_ADD_WARNING(92, errorKey,
				"Mounting modules did not work");
	}

	elektraMountVersion (handle, errorKey);

	// elektraSplitOpen(handle->split);

	keyDel (initialParent);
	errno = errnosave;
	return handle;
}


/**
 * Closes the session with the Key database.
 *
 * @pre The handle must be a valid handle as returned from kdbOpen()
 *
 * @pre errorKey must be a valid key, e.g. created with keyNew()
 *
 * This is the counterpart of kdbOpen().
 *
 * You must call this method when you finished your affairs with the key
 * database. You can manipulate Key and KeySet objects also after
 * kdbClose(), but you must not use any kdb*() call afterwards.
 *
 * The @p handle parameter will be finalized and all resources associated to it
 * will be freed. After a kdbClose(), the @p handle cannot be used anymore.
 *
 * @param handle contains internal information of
 *               @link kdbOpen() opened @endlink key database
 * @param errorKey the key which holds error/warning information
 * @retval 0 on success
 * @retval -1 on NULL pointer
 * @ingroup kdb
 */
int kdbClose(KDB *handle, Key *errorKey)
{
	if (!handle)
	{
		return -1;
	}

	int errnosave = errno;
	elektraSplitDel (handle->split);

	elektraTrieClose(handle->trie, errorKey);

	elektraBackendClose (handle->defaultBackend, errorKey);
	handle->defaultBackend = 0;

	if (handle->modules)
	{
		elektraModulesClose (handle->modules, errorKey);
		ksDel (handle->modules);
	}
	else
	{
		ELEKTRA_ADD_WARNING(47, errorKey,
				"modules were not open");
	}

	elektraFree(handle);

	errno = errnosave;
	return 0;
}

/**
 * @internal
 *
 * @brief Check if an update is needed at all
 *
 * @retval -1 an error occurred
 * @retval 0 no update needed
 * @retval number of plugins which need update
 */
static int elektraGetCheckUpdateNeeded(Split *split, Key *parentKey)
{
	int updateNeededOccurred = 0;
	for (size_t i=0; i<split->size;i++)
	{
		int ret = 0;
		Backend *backend = split->handles[i];
		clear_bit(split->syncbits[i], 1);

		if (backend->getplugins[0])
		{
			ksRewind(split->keysets[i]);
			keySetName(parentKey,
					keyName(split->parents[i]));
			ret = backend->getplugins[0]->kdbGet (
					backend->getplugins[0],
					split->keysets[i],
					parentKey);
			// store resolved filename
			keySetString(split->parents[i],
					keyString(parentKey));
			// no keys in that backend
			elektraBackendUpdateSize(backend, split->parents[i], 0);
		}
		switch (ret)
		{
		case 1:
			// Seems like we need to sync that
			set_bit(split->syncbits[i], SPLIT_FLAG_SYNC);
			++ updateNeededOccurred;
			break;
		case 0:
			// Nothing to do here
			break;
		default:
			ELEKTRA_ASSERT(0 && "resolver did not return 1 0 -1");
		case -1:
			// Ohh, an error occurred, lets stop the
			// process.
			return -1;
		}
	}
	return updateNeededOccurred;
}

/**
 * @internal
 * @brief Do the real update.
 *
 * @retval -1 on error
 * @retval 0 on success
 */
static int elektraGetDoUpdate(Split *split, Key *parentKey)
{
	const int bypassedSplits = 1;
	for (size_t i=0; i<split->size-bypassedSplits;i++)
	{
		if (!test_bit(split->syncbits[i], SPLIT_FLAG_SYNC))
		{
			// skip it, update is not needed
			continue;
		}
		Backend *backend = split->handles[i];
		ksRewind (split->keysets[i]);
		keySetName (parentKey, keyName(split->parents[i]));
		keySetString(parentKey,
				keyString(split->parents[i]));

		for (size_t p=1; p<NR_OF_PLUGINS; ++p)
		{
			int ret = 0;
			if (backend->getplugins[p])
			{
				ret = backend->getplugins[p]->kdbGet(
						backend->getplugins[p],
						split->keysets[i],
						parentKey);
			}
			if (ret == -1)
			{
				// Ohh, an error occurred,
				// lets stop the process.
				return -1;
			}
		}
	}
	return 0;
}



/**
 * @brief Retrieve keys in an atomic and universal way.
 *
 * @pre The @p handle must be passed as returned from kdbOpen()
 *
 * @pre The @p returned KeySet must be a valid KeySet, e.g. constructed
 *     with ksNew().
 *
 * @pre The @p parentKey Key must be a valid Key, e.g. constructed with
 *     keyNew().
 *
 * If you pass NULL, which violates the preconditions,
 * on any parameter kdbGet() will fail immediately without doing anything.
 *
 * The @p returned KeySet may already contain some keys, e.g. from previous
 * kdbGet() calls. The new retrieved keys will be appended using
 * ksAppendKey().
 *
 * It will fully retrieve, at least, all keys under the @p parentKey
 * folder, with all subfolders and their children.
 *
 * @note kdbGet() might retrieve more keys then requested (that are not
 *     below parentKey). These keys must be passed to calls of kdbSet(),
 *     otherwise they will be lost. This stems from the fact that the
 *     user has the only copy of the whole configuration and backends
 *     only write configuration that was passed to them.
 *     For example, if you kdbGet() "system/mountpoint/interest"
 *     you will not only get all keys below system/mountpoint/interest,
 *     but also all keys below system/mountpoint (if system/mountpoint
 *     is a mountpoint as the name suggests, but
 *     system/mountpoint/interest is not a mountpoint).
 *     Make sure to not touch or remove keys outside the keys of interest,
 *     because others may need them!
 *
 * @par Example:
 * This example demonstrates the typical usecase within an application
 * (without error handling).
 *
 * @include kdbget.c
 *
 * When a backend fails kdbGet() will return -1 with all
 * error and warning information in the @p parentKey.
 * The parameter @p returned will not be changed.
 *
 * @par Updates:
 * In the first run of kdbGet all requested (or more) keys are retrieved. On subsequent
 * calls only the keys are retrieved where something was changed
 * inside the key database. The other keys stay unchanged in the
 * keyset, even if they were manipulated.
 *
 * It is your responsibility to save the original keyset if you
 * need it afterwards.
 *
 * If you want to get the same keyset again, you need to open a
 * second handle to the key database using kdbOpen().
 *
 * @param handle contains internal information of @link kdbOpen() opened @endlink key database
 * @param parentKey is used to add warnings and set an error
 *         information. Additionally, its name is an hint which keys
 *         should be retrieved (it is possible that more are retrieved).
 *           - cascading keys (starting with /) will retrieve the same path in all namespaces
 *           - / will retrieve all keys
 * @param ks the (pre-initialized) KeySet returned with all keys found
 * 	will not be changed on error or if no update is required
 * @see ksLookup(), ksLookupByName() for powerful
 * 	lookups after the KeySet was retrieved
 * @see kdbOpen() which needs to be called before
 * @see kdbSet() to save the configuration afterwards and kdbClose() to
 * 	finish affairs with the key database.
 * @retval 1 if the keys were retrieved successfully
 * @retval 0 if there was no update - no changes are made to the keyset then
 * @retval -1 on failure - no changes are made to the keyset then
 * @ingroup kdb
 */
int kdbGet(KDB *handle, KeySet *ks, Key *parentKey)
{
	elektraNamespace ns = keyGetNamespace(parentKey);
	if (ns == KEY_NS_NONE)
	{
		return -1;
	}

	if (ns == KEY_NS_META)
	{
		ELEKTRA_SET_ERRORF(104, parentKey,
				"metakey with name \"%s\" passed to kdbGet",
				keyName(parentKey));
		return -1;
	}

	if (ns == KEY_NS_EMPTY)
	{
		ELEKTRA_ADD_WARNING(105, parentKey,
				"invalid key name passed to kdbGet");
	}

	int errnosave = errno;
	Key *initialParent = keyDup (parentKey);

#if DEBUG && VERBOSE
	fprintf (stderr, "now in new kdbGet (%s)\n",
			keyName(parentKey));
#endif

	Split *split = elektraSplitNew();

	if(!handle || !ks)
	{
		ELEKTRA_SET_ERROR(37, parentKey,
				"handle or ks null pointer");
		goto error;
	}

	if(elektraSplitBuildup (split, handle, parentKey) == -1)
	{
		ELEKTRA_SET_ERROR(38, parentKey,
				"error in elektraSplitBuildup");
		goto error;
	}

	// Check if a update is needed at all
	switch(elektraGetCheckUpdateNeeded(split, parentKey))
	{
	case 0: // We don't need an update so let's do nothing
		keySetName (parentKey, keyName(initialParent));
		elektraSplitUpdateFileName(split, handle, parentKey);
		keyDel (initialParent);
		elektraSplitDel (split);
		errno = errnosave;
		return 0;
	case -1: goto error;
	// otherwise falltrough
	}

	// Appoint keys (some in the bypass)
	if(elektraSplitAppoint (split, handle, ks) == -1)
	{
		ELEKTRA_SET_ERROR (38, parentKey, "error in elektraSplitAppoint");
		goto error;
	}

	/* Now do the real updating,
	  but not for bypassed keys in split->size-1 */
	if(elektraGetDoUpdate(split, parentKey) == -1)
	{
		goto error;
	}

	/* Now postprocess the updated keysets */
	if (elektraSplitGet (split, parentKey, handle) == -1)
	{
		ELEKTRA_ADD_WARNING(108, parentKey, keyName(ksCurrent(ks)));
		// continue, because sizes are already updated
	}

	/* We are finished, now just merge everything to returned */
	ksClear (ks);
	elektraSplitMerge (split, ks);

	keySetName (parentKey, keyName(initialParent));
	elektraSplitUpdateFileName(split, handle, parentKey);
	keyDel (initialParent);
	elektraSplitDel (split);
	errno = errnosave;
	return 1;

error:
	keySetName (parentKey, keyName(initialParent));
	elektraSplitUpdateFileName(split, handle, parentKey);
	keyDel (initialParent);
	elektraSplitDel (split);
	errno = errnosave;
	return -1;
}

/**
 * @internal
 * @brief Does all set steps but not commit
 *
 * @param split all information for iteration
 * @param parentKey to add warnings (also passed to plugins for the same reason)
 * @param [out] errorKey may point to which key caused the error or 0 otherwise
 *
 * @retval -1 on error
 * @retval 0 on success
 */
static int elektraSetPrepare(Split *split, Key *parentKey, Key **errorKey)
{
	int any_error = 0;
	for(size_t i=0; i<split->size;i++)
	{
		for(size_t p=0; p<COMMIT_PLUGIN; ++p)
		{
			int ret = 0; // last return value

			Backend *backend = split->handles[i];
			ksRewind (split->keysets[i]);
			if(backend->setplugins[p])
			{
				if(p != 0)
				{
					keySetString (parentKey,
						keyString(split->parents[i]));
				}
				keySetName (parentKey,
					keyName(split->parents[i]));
				ret = backend->setplugins[p]->kdbSet (
						backend->setplugins[p],
						split->keysets[i],
						parentKey);

#if VERBOSE && DEBUG
				printf ("Prepare %s with keys %zd in plugin: %zu, split: %zu, ret: %d\n",
						keyName(parentKey), ksGetSize(split->keysets[i]), p, i, ret);
#endif

				if(p == 0)
				{
					if (ret == 0)
					{
						// resolver says that sync is
						// not needed, so we
						// skip other pre-commit
						// plugins
						break;
					}
					keySetString (split->parents[i],
						keyString(parentKey));
				}
			}
			if (ret == -1)
			{
				// do not
				// abort because it might
				// corrupt the KeySet
				// and leads to warnings
				// because of .tmp files not
				// found
				*errorKey =
					ksCurrent(split->keysets[i]);

				// so better keep going, but of
				// course we will not commit
				any_error = -1;
			}
		}
	}
	return any_error;
}

/**
 * @internal
 * @brief Does the commit
 *
 * @param split all information for iteration
 * @param parentKey to add warnings (also passed to plugins for the same reason)
 */
static void elektraSetCommit(Split *split, Key *parentKey)
{
	for(size_t p=COMMIT_PLUGIN; p<NR_OF_PLUGINS; ++p)
	{
		for(size_t i=0; i<split->size;i++)
		{
			int ret = 0;
			Backend *backend = split->handles[i];

			if(backend->setplugins[p])
			{
				if (p!=COMMIT_PLUGIN)
				{
					keySetString(parentKey,
						keyString(split->parents[i]));
				}
				keySetName(parentKey, keyName(split->parents[i]));
#if DEBUG && VERBOSE
				printf ("elektraSetCommit: %p # %zu with %s - %s\n",
						backend, p, keyName(parentKey), keyString(parentKey));
#endif
				ksRewind (split->keysets[i]);
				ret = backend->setplugins[p]->kdbSet (
						backend->setplugins[p],
						split->keysets[i],
						parentKey);
				if (p==COMMIT_PLUGIN)
				{
					// name of non-temp file
					keySetString (split->parents[i],
						keyString(parentKey));
				}
			}

			if (ret == -1)
			{
				ELEKTRA_ADD_WARNING(80, parentKey,
						keyName(backend->mountpoint));
			}
		}
	}
}

/**
 * @internal
 * @brief Does the rollback
 *
 * @param split all information for iteration
 * @param parentKey to add warnings (also passed to plugins for the same reason)
 */
static void elektraSetRollback(Split *split, Key *parentKey)
{
	for(size_t p=0; p<NR_OF_PLUGINS; ++p)
	{
		for(size_t i=0; i<split->size; i++)
		{
			int ret = 0;
			Backend *backend = split->handles[i];

			ksRewind (split->keysets[i]);
			if(backend->errorplugins[p])
			{
				keySetName (parentKey, keyName(split->parents[i]));
				ret = backend->errorplugins[p]->kdbError (
						backend->errorplugins[p],
						split->keysets[i],
						parentKey);
			}

			if(ret == -1)
			{
				ELEKTRA_ADD_WARNING(81, parentKey,
						keyName(backend->mountpoint));
			}
		}
	}
}


/** @brief Set keys in an atomic and universal way.
 *
 * @pre kdbGet() must be called before kdbSet():
 *    - initially (after kdbOpen())
 *    - after conflict errors in kdbSet().
 *
 * @pre The @p returned KeySet must be a valid KeySet, e.g. constructed
 *     with ksNew().
 *
 * @pre The @p parentKey Key must be a valid Key, e.g. constructed with
 *
 * With @p parentKey you can give an hint which part of the given keyset
 * is of interest for you. Then you promise, you only modified or
 * removed keys below this key.
 *
 * @par Errors
 * If some error occurs, kdbSet() will stop. In this situation the KeySet
 * internal cursor will be set on the key that generated the error.
 * None of the keys are actually committed in this situation (no
 * configuration file will be modified).
 *
 * In case of errors you should present the error message to the user and let the user decide what
 * to do. Possible solutions are:
 * - remove the problematic key and use kdbSet() again (for validation or type errors)
 * - change the value of the problematic key and use kdbSet() again (for validation errors)
 * - do a kdbGet() (for conflicts, i.e. error 30) and then
 *   - set the same keyset again (in favour of what was set by this user)
 *   - drop the old keyset (in favour of what was set from another application)
 *   - merge the original, your own and the other keyset
 * - export the configuration into a file (for unresolvable errors)
 * - repeat the same kdbSet might be of limited use if the operator does
 *   not explicitly request it, because temporary
 *   errors are rare and its unlikely that they fix themselves
 *   (e.g. disc full, permission problems)
 *
 * @par Optimization
 * Each key is checked with keyNeedSync() before being actually committed. So
 * only changed keys are updated. If no key of a backend needs to be synced
 * any affairs to backends are omitted and 0 is returned.
 *
 * @snippet kdbset.c set
 *
 * showElektraErrorDialog() and doElektraMerge() need to be implemented
 * by the user of Elektra. For doElektraMerge a 3-way merge algorithm exists in
 * libelektra-tools.
 *
 * @param handle contains internal information of @link kdbOpen() opened @endlink key database
 * @param ks a KeySet which should contain changed keys, otherwise nothing is done
 * @param parentKey is used to add warnings and set an error
 *         information. Additionally, its name is an hint which keys
 *         should be committed (it is possible that more are changed).
 *           - cascading keys (starting with /) will set the path in all namespaces
 *           - / will commit all keys
 *           - meta-names will be rejected (error 104)
 *           - empty/invalid (error 105)
 * @retval 1 on success
 * @retval 0 if nothing had to be done, no changes in KDB
 * @retval -1 on failure, no changes in KDB
 * @see keyNeedSync()
 * @see ksCurrent() contains the error key
 * @see kdbOpen() and kdbGet() that must be called first
 * @see kdbClose() that must be called afterwards
 * @ingroup kdb
 */
int kdbSet(KDB *handle, KeySet *ks, Key *parentKey)
{
	elektraNamespace ns = keyGetNamespace(parentKey);
	if (ns == KEY_NS_NONE)
	{
		return -1;
	}

	if (ns == KEY_NS_META)
	{
		ELEKTRA_SET_ERRORF(104, parentKey,
				"metakey with name \"%s\" passed to kdbSet",
				keyName(parentKey));
		return -1;
	}

	if (ns == KEY_NS_EMPTY)
	{
		ELEKTRA_ADD_WARNING(105, parentKey,
				"invalid key name passed to kdbSet");
	}

	if(!handle || !ks)
	{
		ELEKTRA_SET_ERROR (37, parentKey, "handle or ks null pointer");
		return -1;
	}

	int errnosave = errno;
	Key *initialParent = keyDup(parentKey);

#if DEBUG && VERBOSE
	fprintf(stderr, "now in new kdbSet (%s)\n", keyName(parentKey));
#endif

	Split *split = elektraSplitNew();
	Key *errorKey = 0;

	if(elektraSplitBuildup(split, handle, parentKey) == -1)
	{
		ELEKTRA_SET_ERROR(38, parentKey, "error in elektraSplitBuildup");
		goto error;
	}

	// 1.) Search for syncbits
	int syncstate = elektraSplitDivide(split, handle, ks);
	if(syncstate == -1)
	{
		ELEKTRA_SET_ERROR(8, parentKey, keyName(ksCurrent(ks)));
		goto error;
	}
	ELEKTRA_ASSERT(syncstate == 0 || syncstate == 1);

	// 2.) Search for changed sizes
	syncstate |= elektraSplitSync(split);
	ELEKTRA_ASSERT(syncstate == 0 || syncstate == 1);
	if (syncstate != 1)
	{
		/* No update is needed */
		keySetName (parentKey, keyName(initialParent));
		if(syncstate == -1)
		{
			ELEKTRA_SET_ERROR(107, parentKey, "Assert failed: invalid namespace");
		}
		else if(syncstate < -1)
		{
			ELEKTRA_SET_ERROR(107, parentKey,
					 keyName(split->parents[-syncstate-2]));
		}
		keyDel (initialParent);
		elektraSplitDel (split);
		errno = errnosave;
		return syncstate == 0 ? 0 : -1;
	}
	ELEKTRA_ASSERT(syncstate == 1);

	elektraSplitPrepare(split);

	if (elektraSetPrepare(split, parentKey, &errorKey) == -1)
	{
		goto error;
	}

	elektraSetCommit(split, parentKey);

	elektraSplitUpdateSize(split);

	for (size_t i=0; i<ks->size; ++i)
	{
		// remove all flags from all keys
		clear_bit(ks->array[i]->flags, KEY_FLAG_SYNC);
	}

	keySetName(parentKey, keyName(initialParent));
	keyDel(initialParent);
	elektraSplitDel(split);

	errno = errnosave;
	return 1;

error:
	elektraSetRollback(split, parentKey);

	if(errorKey)
	{
		Key *found = ksLookup(ks, errorKey, 0);
		if (!found)
		{
			ELEKTRA_ADD_WARNING(82, parentKey,
					keyName(errorKey));
		}
	}

	keySetName(parentKey, keyName(initialParent));
	keyDel(initialParent);
	elektraSplitDel(split);
	errno = errnosave;
	return -1;
}

/**
 * @}
 */
