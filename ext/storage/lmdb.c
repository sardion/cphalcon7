
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  |          ZhuZongXin <dreamsxin@qq.com>                                 |
  +------------------------------------------------------------------------+
*/

#include "storage/lmdb.h"
#include "storage/exception.h"

#include <ext/standard/file.h>
#include <zend_smart_str.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/array.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/operators.h"
#include "kernel/file.h"
#include "kernel/exception.h"

/**
 * Phalcon\Storage\Lmdb
 *
 * It can be used to replace APC or local memstoraged.
 */
zend_class_entry *phalcon_storage_lmdb_ce;

PHP_METHOD(Phalcon_Storage_Lmdb, __construct);
PHP_METHOD(Phalcon_Storage_Lmdb, begin);
PHP_METHOD(Phalcon_Storage_Lmdb, commit);
PHP_METHOD(Phalcon_Storage_Lmdb, abort);
PHP_METHOD(Phalcon_Storage_Lmdb, getAll);
PHP_METHOD(Phalcon_Storage_Lmdb, get);
PHP_METHOD(Phalcon_Storage_Lmdb, put);
PHP_METHOD(Phalcon_Storage_Lmdb, del);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_storage_lmdb___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, readers, IS_LONG, 1)
	ZEND_ARG_TYPE_INFO(0, mapsize, IS_LONG, 1)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_storage_lmdb_begin, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_storage_lmdb_get, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_storage_lmdb_put, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_storage_lmdb_del, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_storage_lmdb_method_entry[] = {
	PHP_ME(Phalcon_Storage_Lmdb, __construct, arginfo_phalcon_storage_lmdb___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Storage_Lmdb, begin, arginfo_phalcon_storage_lmdb_begin, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, commit, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, abort, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, getAll, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, get, arginfo_phalcon_storage_lmdb_get, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, put, arginfo_phalcon_storage_lmdb_put, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Storage_Lmdb, del, arginfo_phalcon_storage_lmdb_del, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

zend_object_handlers phalcon_storage_lmdb_object_handlers;
zend_object* phalcon_storage_lmdb_object_create_handler(zend_class_entry *ce)
{
	phalcon_storage_lmdb_object *intern = ecalloc(1, sizeof(phalcon_storage_lmdb_object) + zend_object_properties_size(ce));
	intern->std.ce = ce;

	zend_object_std_init(&intern->std, ce);
	object_properties_init(&intern->std, ce);
	intern->std.handlers = &phalcon_storage_lmdb_object_handlers;

	return &intern->std;
}

void phalcon_storage_lmdb_object_free_handler(zend_object *object)
{
	phalcon_storage_lmdb_object *intern;

	intern = phalcon_storage_lmdb_object_from_obj(object);
	if (intern->env) {
		mdb_env_close(intern->env);
		intern->env = NULL;
	}
}

/**
 * Phalcon\Storage\Lmdb initializer
 */
PHALCON_INIT_CLASS(Phalcon_Storage_Lmdb){

	PHALCON_REGISTER_CLASS_CREATE_OBJECT(Phalcon\\Storage, Lmdb, storage_lmdb, phalcon_storage_lmdb_method_entry, 0);

	zend_declare_property_null(phalcon_storage_lmdb_ce, SL("_path"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_storage_lmdb_ce, SL("_name"), ZEND_ACC_PROTECTED);
	zend_declare_property_long(phalcon_storage_lmdb_ce, SL("_flag"), 0, ZEND_ACC_PROTECTED);

	// Environment Flags
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("FIXEDMAP"),	MDB_FIXEDMAP);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOSUBDIR"),	MDB_NOSUBDIR);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOSYNC"),		MDB_NOSYNC);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("RDONLY"),		MDB_RDONLY);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOMETASYNC"),	MDB_NOMETASYNC);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("WRITEMAP"),	MDB_WRITEMAP);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("MAPASYNC"),	MDB_MAPASYNC);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOTLS"),		MDB_NOTLS);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOLOCK"),		MDB_NOLOCK);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NORDAHEAD"),	MDB_NORDAHEAD);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOMEMINIT"),	MDB_NOMEMINIT);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("PREVMETA"),	MDB_PREVMETA);

	// Database Flags
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("REVERSEKEY"),	MDB_REVERSEKEY);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("DUPSORT"),	MDB_DUPSORT);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("INTEGERKEY"),	MDB_INTEGERKEY);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("DUPFIXED"),	MDB_DUPFIXED);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("INTEGERDUP"),	MDB_INTEGERDUP);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("REVERSEDUP"),	MDB_REVERSEDUP);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("CREATE"),		MDB_CREATE);

	// Write Flags
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOOVERWRITE"),	MDB_NOOVERWRITE);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NODUPDATA"),		MDB_NODUPDATA);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("CURRENT"),		MDB_CURRENT);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("RESERVE"),		MDB_RESERVE);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("APPEND"),			MDB_APPEND);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("APPENDDUP"),		MDB_APPENDDUP);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("MULTIPLE"),		MDB_MULTIPLE);

	// Copy Flags
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("CP_COMPACT"),		MDB_CP_COMPACT);

	// Return Codes
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("SUCCESS"),			MDB_SUCCESS);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("KEYEXIST"),			MDB_KEYEXIST);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("NOTFOUND"),			MDB_NOTFOUND);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("PAGE_NOTFOUND"),		MDB_PAGE_NOTFOUND);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("CORRUPTED"),			MDB_CORRUPTED);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("PANIC"),				MDB_PANIC);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("VERSION_MISMATCH"),	MDB_VERSION_MISMATCH);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("INVALID"),			MDB_INVALID);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("MAP_FULL"),			MDB_MAP_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("DBS_FULL"),			MDB_DBS_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("READERS_FULL"),		MDB_READERS_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("TLS_FULL"),			MDB_TLS_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("TXN_FULL"),			MDB_TXN_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("CURSOR_FULL"),		MDB_CURSOR_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("PAGE_FULL"),			MDB_PAGE_FULL);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("MAP_RESIZED"),		MDB_MAP_RESIZED);

	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("INCOMPATIBLE"),		MDB_INCOMPATIBLE);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("BAD_RSLOT"),			MDB_BAD_RSLOT);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("BAD_TXN"),			MDB_BAD_TXN);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("BAD_VALSIZE"),		MDB_BAD_VALSIZE);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("BAD_DBI"),			MDB_BAD_DBI);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("PROBLEM"),			MDB_PROBLEM);
	zend_declare_class_constant_long(phalcon_storage_lmdb_ce, SL("LAST_ERRCODE"),		MDB_LAST_ERRCODE);

	return SUCCESS;
}

/**
 * Phalcon\Storage\Lmdb constructor
 *
 * @param string $path
 * @param string $name
 * @param int $readers
 * @param int $mapsize
 */
PHP_METHOD(Phalcon_Storage_Lmdb, __construct)
{
	zval *path, *name = NULL, *readers = NULL, *mapsize = NULL, *flags = NULL;
	phalcon_storage_lmdb_object *intern;
	int rc;

	phalcon_fetch_params(0, 1, 4, &path, &name, &readers, &mapsize, &flags);

	if (phalcon_file_exists(path) == FAILURE) {
		php_mkdir(Z_STRVAL_P(path), 0775);
	}

	phalcon_update_property(getThis(), SL("_path"), path);

	if (name && Z_TYPE_P(name) == IS_STRING) {
		phalcon_update_property(getThis(), SL("_name"), name);
	}

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_env_create(&intern->env);
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_storage_exception_ce, "Failed to create an LMDB environment handle");
		return;
	}
	if (readers && Z_TYPE_P(readers) == IS_LONG) {
		mdb_env_set_maxreaders(intern->env, Z_LVAL_P(readers));
	}
	if (mapsize && Z_TYPE_P(mapsize) == IS_LONG) {
		mdb_env_set_mapsize(intern->env, Z_LVAL_P(mapsize));
	}
	if (flags && Z_TYPE_P(flags) == IS_LONG) {
		rc = mdb_env_open(intern->env, Z_STRVAL_P(path), Z_LVAL_P(flags), 0664);
	} else {
		rc = mdb_env_open(intern->env, Z_STRVAL_P(path), 0, 0664);
	}
	
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to open an environment handle (%s)", Z_STRVAL_P(path));
		return;
	}
}

/**
 * Create a transaction for use with the environment
 *
 */
PHP_METHOD(Phalcon_Storage_Lmdb, begin)
{
	zval *flags = NULL, name = {};
	phalcon_storage_lmdb_object *intern;
	int rc;

	phalcon_fetch_params(0, 0, 1, &flags);

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));
	if (flags && Z_TYPE_P(flags) == IS_LONG) {
		rc = mdb_txn_begin(intern->env, NULL, Z_LVAL_P(flags), &intern->txn);
	} else {
		rc = mdb_txn_begin(intern->env, NULL, 0, &intern->txn);
	}
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to create a transaction for use with the environment (%s)", mdb_strerror(rc));
		return;
	}

	phalcon_read_property(&name, getThis(), SL("_name"), PH_NOISY|PH_READONLY);
	if (Z_TYPE(name) == IS_STRING) {
		rc = mdb_dbi_open(intern->txn, Z_STRVAL(name), 0, &intern->dbi);
	} else {
		rc = mdb_dbi_open(intern->txn, NULL, 0, &intern->dbi);
	}
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_storage_exception_ce, "Failed to open a database in the environment");
		return;
	}
}

/**
 * Commit all the operations of a transaction into the database
 *
 */
PHP_METHOD(Phalcon_Storage_Lmdb, commit)
{
	phalcon_storage_lmdb_object *intern;
	int rc;

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_txn_commit(intern->txn);
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to commit all the operations of a transaction into the database (%s)", mdb_strerror(rc));
		return;
	}
}

/**
 * Abandon all the operations of the transaction instead of saving them
 *
 */
PHP_METHOD(Phalcon_Storage_Lmdb, abort)
{
	phalcon_storage_lmdb_object *intern;

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	mdb_dbi_close(intern->env, intern->dbi);
	mdb_txn_abort(intern->txn);
}

/**
 * Get all items from a database
 *
 * @return array
 */
PHP_METHOD(Phalcon_Storage_Lmdb, getAll)
{
	MDB_val k, v;
	MDB_cursor *cursor;
	phalcon_storage_lmdb_object *intern;
	int rc;

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_cursor_open(intern->txn, intern->dbi, &cursor);
	if (rc != MDB_SUCCESS) {
		mdb_dbi_close(intern->env, intern->dbi);
		mdb_txn_abort(intern->txn);
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to create a cursor handle (%s)", mdb_strerror(rc));
		return;
	}
	array_init(return_value);
	while ((rc = mdb_cursor_get(cursor, &k, &v, MDB_NEXT)) == 0) {
		phalcon_array_update_str_str(return_value, (char *) k.mv_data, (int) k.mv_size, (char *) v.mv_data, (int) v.mv_size, 0);
	}
	mdb_cursor_close(cursor);
}

/**
 * Get items from a database
 *
 * @param string $key
 * @return string
 */
PHP_METHOD(Phalcon_Storage_Lmdb, get)
{
	zval *key;
	MDB_val k, v;
	phalcon_storage_lmdb_object *intern;
	int rc;

	phalcon_fetch_params(0, 1, 0, &key);

	k.mv_size = Z_STRLEN_P(key);
	k.mv_data = Z_STRVAL_P(key);

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_get(intern->txn, intern->dbi, &k, &v);
	if (rc == MDB_SUCCESS) {
		ZVAL_STRINGL(return_value, (char *) v.mv_data, (int) v.mv_size);
	} else if (rc == MDB_NOTFOUND) {
		RETVAL_FALSE;
	} else {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to get items from a database (%s)", mdb_strerror(rc));
		return;
	}
}

/**
 * Store items into a database
 *
 * @param string $key
 * @param string $value
 * @return mixed
 */
PHP_METHOD(Phalcon_Storage_Lmdb, put)
{
	zval *key, *value;
	MDB_val k, v;
	phalcon_storage_lmdb_object *intern;
	int rc;

	phalcon_fetch_params(0, 2, 0, &key, &value);

	k.mv_size = Z_STRLEN_P(key);
	k.mv_data = Z_STRVAL_P(key);
	v.mv_size = Z_STRLEN_P(value);
	v.mv_data = Z_STRVAL_P(value);

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_put(intern->txn, intern->dbi, &k, &v, 0);
	if (rc != MDB_SUCCESS) {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to store items into a database (%s)", mdb_strerror(rc));
		return;
	}

	RETURN_TRUE;
}

/**
 * Delete items from a database
 *
 * @param string $keys
 * @return boolean
 */
PHP_METHOD(Phalcon_Storage_Lmdb, del)
{
	zval *key;
	MDB_val k;
	phalcon_storage_lmdb_object *intern;
	int rc;

	phalcon_fetch_params(0, 1, 0, &key);

	k.mv_size = Z_STRLEN_P(key);
	k.mv_data = Z_STRVAL_P(key);

	intern = phalcon_storage_lmdb_object_from_obj(Z_OBJ_P(getThis()));

	rc = mdb_del(intern->txn, intern->dbi, &k, NULL);
	if (rc == MDB_SUCCESS) {
		RETVAL_TRUE;
	} else if (rc == MDB_NOTFOUND) {
		RETVAL_FALSE;
	} else {
		PHALCON_THROW_EXCEPTION_FORMAT(phalcon_storage_exception_ce, "Failed to Delete items from a database (%s)", mdb_strerror(rc));
		return;
	}
}