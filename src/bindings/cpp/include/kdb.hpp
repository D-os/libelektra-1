#ifndef ELEKTRA_KDB_HPP
#define ELEKTRA_KDB_HPP

#include <string>

#include <key.hpp>
#include <keyset.hpp>
#include <kdbexcept.hpp>

#include <kdb.h>


namespace kdb
{

/**
 * @copydoc KDB
 *
 * @brief Access to the key database.
 *
 * @invariant the object holds an valid connection to the key database
 * or is empty
 */
class KDB
{
public:
	KDB ();
	KDB (Key & errorKey);
	~KDB () throw();

	void open(Key & errorKey);
	void close(Key & errorKey) throw();

	inline int get (KeySet & returned, std::string const & keyname);
	inline int get (KeySet & returned, Key & parentKey);
	inline int set (KeySet & returned, std::string const & keyname);
	inline int set (KeySet & returned, Key & parentKey);

private:
	ckdb::KDB* handle; ///< holds an kdb handle
};

/**
 * Constructs a class KDB.
 *
 * @throw KDBException if database could not be opened
 *
 * @copydoc kdbOpen
 */
inline KDB::KDB ()
{
	Key errorKey;
	open(errorKey);
}

/**
 * Constructs a class KDB.
 *
 * @param errorKey is useful if you want to get the warnings in
 * the successful case, when no exception is thrown.
 *
 * @throw KDBException if database could not be opened
 *
 * @copydoc kdbOpen
 */
inline KDB::KDB (Key &errorKey)
{
	open(errorKey);
}

/**
 * The destructor closes the database.
 *
 * @copydoc kdbClose
 */
inline KDB::~KDB () throw()
{
	Key errorKey;
	try {
		close (errorKey);
	}
	catch (...)
	{
		// silently drop potential warnings/errors
	}
}

/**
 * Open the database
 *
 * @param errorKey is useful if you want to get the warnings in
 * the successful case, when no exception is thrown.
 *
 * @copydoc kdbOpen
 */
inline void KDB::open (Key &errorKey)
{
	handle = ckdb::kdbOpen(errorKey.getKey());
	if (!handle)
	{
		throw kdb::KDBException(errorKey);
	}
}

/**
 * Open the database.
 *
 * The return value does not matter because its only a null pointer check.
 *
 * @param errorKey is useful if you want to get the warnings
 *
 * @copydoc kdbClose
 */
inline void KDB::close (Key & errorKey) throw()
{
	ckdb::kdbClose(handle, errorKey.getKey());
	handle = 0;
}

/**
 * @class doxygenKDBReturn
 * @brief
 *
 * @retval 0 if no key was updated
 * @retval 1 if user or system keys were updated
 * @retval 2 if user and system keys were updated
 */

/**
 * Get all keys below keyname inside returned.
 *
 * @copydoc kdbGet
 *
 * @include cpp_example_get.cpp
 *
 * @param returned the keyset where the keys will be in
 * @param keyname the root keyname which should be used to get keys below it
 *
 * @copydetails doxygenKDBReturn
 *
 * @throw KDBException if there were problems with the database
 *
 * @see KDB::get (KeySet & returned, Key & parentKey)
 */
inline int KDB::get (KeySet & returned, std::string const & keyname)
{
	int ret=0;
	if (keyname[0] != '/')
	{
		Key parentKey (keyname.c_str(), KEY_END);
		ret += get (returned, parentKey);
	}
	else
	{
		std::string userKeyname = "user" + keyname;
		Key userParentKey (userKeyname.c_str(), KEY_END);
		ret += get (returned, userParentKey);

		std::string systemKeyname = "system" + keyname;
		Key systemParentKey (systemKeyname.c_str(), KEY_END);
		ret += get (returned, systemParentKey);
	}
	return ret;
}

/**
 * Get all keys below parentKey inside returned.
 *
 * @copydoc kdbGet
 *
 * @param returned the keyset where the keys will be in
 * @param parentKey the parentKey of returned
 *
 * @copydetails doxygenKDBReturn
 *
 * @throw KDBException if there were problems with the database
 */
inline int KDB::get (KeySet & returned, Key & parentKey)
{
	int ret = ckdb::kdbGet (handle, returned.getKeySet(), parentKey.getKey());
	if (ret == -1)
	{
		throw KDBException(parentKey);
	}
	return ret;
}

/**
 * Set all keys below keyname.
 *
 * If the keyname of the parentKey is invalid (e.g. empty) all keys will be set.
 *
 * @copydoc kdbSet
 *
 * @copydetails doxygenKDBReturn
 *
 * @param returned the keyset where the keys will be in
 * @param keyname the keyname below the names should be set
 *
 * @throw KDBException if there were problems with the database
 */
inline int KDB::set (KeySet & returned, std::string const & keyname)
{
	int ret = 0;
	if (keyname[0] != '/')
	{
		Key parentKey (keyname.c_str(), KEY_END);
		ret += set (returned, parentKey);
	}
	else
	{
		std::string userKeyname = "user" + keyname;
		Key userParentKey (userKeyname.c_str(), KEY_END);
		ret += set (returned, userParentKey);

		std::string systemKeyname = "system" + keyname;
		Key systemParentKey (systemKeyname.c_str(), KEY_END);
		ret += set (returned, systemParentKey);
	}
	return ret;
}

/**
 * Set all keys below parentKey.
 *
 * If the keyname of the parentKey is invalid (e.g. empty) all keys will be set.
 *
 * @copydoc kdbSet
 *
 * @copydetails doxygenKDBReturn
 *
 * @param returned the keyset where the keys are passed to the user
 * @param parentKey the parentKey of returned
 *
 * @throw KDBException if there were problems with the database
 */
inline int KDB::set (KeySet & returned, Key & parentKey)
{
	int ret = ckdb::kdbSet(handle, returned.getKeySet(), parentKey.getKey());
	if (ret == -1)
	{
		throw KDBException(parentKey);
	}
	return ret;
}

} // end of namespace kdb

#endif

