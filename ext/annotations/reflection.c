
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
  +------------------------------------------------------------------------+
*/

#include "php_phalcon.h"

#include "annotations/reflection.h"
#include "annotations/collection.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/array.h"
#include "kernel/fcall.h"
#include "kernel/file.h"
#include "kernel/hash.h"

/**
 * Phalcon\Annotations\Reflection
 *
 * Allows to manipulate the annotations reflection in an OO manner
 *
 *<code>
 * //Parse the annotations in a class
 * $reader = new \Phalcon\Annotations\Reader();
 * $parsing = $reader->parse('MyComponent');
 *
 * //Create the reflection
 * $reflection = new \Phalcon\Annotations\Reflection($parsing);
 *
 * //Get the annotations in the class docblock
 * $classAnnotations = $reflection->getClassAnnotations();
 *</code>
 */
zend_class_entry *phalcon_annotations_reflection_ce;

PHP_METHOD(Phalcon_Annotations_Reflection, __construct);
PHP_METHOD(Phalcon_Annotations_Reflection, getClassAnnotations);
PHP_METHOD(Phalcon_Annotations_Reflection, getMethodsAnnotations);
PHP_METHOD(Phalcon_Annotations_Reflection, getPropertiesAnnotations);
PHP_METHOD(Phalcon_Annotations_Reflection, getReflectionData);
PHP_METHOD(Phalcon_Annotations_Reflection, __set_state);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_annotations_reflection___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, reflectionData)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_annotations_reflection___set_state, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_annotations_reflection_method_entry[] = {
	PHP_ME(Phalcon_Annotations_Reflection, __construct, arginfo_phalcon_annotations_reflection___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Annotations_Reflection, getClassAnnotations, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Annotations_Reflection, getMethodsAnnotations, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Annotations_Reflection, getPropertiesAnnotations, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Annotations_Reflection, getReflectionData, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Annotations_Reflection, __set_state, arginfo_phalcon_annotations_reflection___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};


/**
 * Phalcon\Annotations\Reflection initializer
 */
PHALCON_INIT_CLASS(Phalcon_Annotations_Reflection){

	PHALCON_REGISTER_CLASS(Phalcon\\Annotations, Reflection, annotations_reflection, phalcon_annotations_reflection_method_entry, 0);

	zend_declare_property_null(phalcon_annotations_reflection_ce, SL("_reflectionData"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_annotations_reflection_ce, SL("_classAnnotations"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_annotations_reflection_ce, SL("_methodAnnotations"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_annotations_reflection_ce, SL("_propertyAnnotations"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}

/**
 * Phalcon\Annotations\Reflection constructor
 *
 * @param array $reflectionData
 */
PHP_METHOD(Phalcon_Annotations_Reflection, __construct){

	zval *reflection_data = NULL;

	phalcon_fetch_params(0, 0, 1, &reflection_data);
	
	if (reflection_data && Z_TYPE_P(reflection_data) == IS_ARRAY) {
		phalcon_update_property_this(getThis(), SL("_reflectionData"), reflection_data);
	}
}

/**
 * Returns the annotations found in the class docblock
 *
 * @return Phalcon\Annotations\Collection
 */
PHP_METHOD(Phalcon_Annotations_Reflection, getClassAnnotations){

	zval *annotations, *reflection_data, *reflection_class;

	PHALCON_MM_GROW();

	annotations = phalcon_read_property(getThis(), SL("_classAnnotations"), PH_NOISY);
	if (Z_TYPE_P(annotations) != IS_OBJECT) {
	
		reflection_data = phalcon_read_property(getThis(), SL("_reflectionData"), PH_NOISY);
		if (phalcon_array_isset_string_fetch(&reflection_class, reflection_data, SS("class"))) {
			object_init_ex(return_value, phalcon_annotations_collection_ce);
			PHALCON_CALL_METHOD(NULL, return_value, "__construct", reflection_class);
	
			phalcon_update_property_this(getThis(), SL("_classAnnotations"), return_value);
			RETURN_MM();
		}
	
		phalcon_update_property_this(getThis(), SL("_classAnnotations"), &PHALCON_GLOBAL(z_false));
		RETURN_MM_FALSE;
	}
	
	RETURN_CTOR(annotations);
}

/**
 * Returns the annotations found in the methods' docblocks
 *
 * @return Phalcon\Annotations\Collection[]
 */
PHP_METHOD(Phalcon_Annotations_Reflection, getMethodsAnnotations){

	zval *annotations, *reflection_data, *reflection_methods;
	zval *reflection_method = NULL;
	zval *collection = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	annotations = phalcon_read_property(getThis(), SL("_methodAnnotations"), PH_NOISY);
	if (Z_TYPE_P(annotations) != IS_OBJECT) {
	
		reflection_data = phalcon_read_property(getThis(), SL("_reflectionData"), PH_NOISY);
		if (phalcon_array_isset_string_fetch(&reflection_methods, reflection_data, SS("methods"))) {
			if (phalcon_fast_count_ev(reflection_methods)) {
				array_init(return_value);

				ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(reflection_methods), idx, str_key, reflection_method) {
					zval method_name;
					if (str_key) {
						ZVAL_STR(&method_name, str_key);
					} else {
						ZVAL_LONG(&method_name, idx);
					}

					PHALCON_INIT_NVAR(collection);
					object_init_ex(collection, phalcon_annotations_collection_ce);
					PHALCON_CALL_METHOD(NULL, collection, "__construct", reflection_method);

					phalcon_array_update_zval(return_value, &method_name, collection, PH_COPY);
				} ZEND_HASH_FOREACH_END();
	
				phalcon_update_property_this(getThis(), SL("_methodAnnotations"), return_value);
	
				RETURN_MM();
			}
		}
	
		phalcon_update_property_bool(getThis(), SL("_methodAnnotations"), 0);
		RETURN_MM_FALSE;
	}
	
	RETURN_CTOR(annotations);
}

/**
 * Returns the annotations found in the properties' docblocks
 *
 * @return Phalcon\Annotations\Collection[]
 */
PHP_METHOD(Phalcon_Annotations_Reflection, getPropertiesAnnotations){

	zval *annotations, *reflection_data, *reflection_properties;
	zval *reflection_property = NULL;
	zval *collection = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	annotations = phalcon_read_property(getThis(), SL("_propertyAnnotations"), PH_NOISY);
	if (Z_TYPE_P(annotations) != IS_OBJECT) {

		reflection_data = phalcon_read_property(getThis(), SL("_reflectionData"), PH_NOISY);
		if (phalcon_array_isset_string_fetch(&reflection_properties, reflection_data, SS("properties"))) {

			if (phalcon_fast_count_ev(reflection_properties)) {
				array_init(return_value);

				ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(reflection_properties), idx, str_key, reflection_property) {
					zval property;
					if (str_key) {
						ZVAL_STR(&property, str_key);
					} else {
						ZVAL_LONG(&property, idx);
					}

					PHALCON_INIT_NVAR(collection);
					object_init_ex(collection, phalcon_annotations_collection_ce);
					PHALCON_CALL_METHOD(NULL, collection, "__construct", reflection_property);

					phalcon_array_update_zval(return_value, &property, collection, PH_COPY);
				} ZEND_HASH_FOREACH_END();

				phalcon_update_property_this(getThis(), SL("_propertyAnnotations"), return_value);
	
				RETURN_MM();
			}
		}

		phalcon_update_property_bool(getThis(), SL("_propertyAnnotations"), 0);
		RETURN_MM_FALSE;
	}

	RETURN_CTOR(annotations);
}

/**
 * Returns the raw parsing intermediate definitions used to construct the reflection
 *
 * @return array
 */
PHP_METHOD(Phalcon_Annotations_Reflection, getReflectionData){


	RETURN_MEMBER(getThis(), "_reflectionData");
}

/**
 * Restores the state of a Phalcon\Annotations\Reflection variable export
 *
 * @return array $data
 */
PHP_METHOD(Phalcon_Annotations_Reflection, __set_state){

	zval *data, *reflection_data;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &data);
	
	if (Z_TYPE_P(data) == IS_ARRAY) { 
	
		/** 
		 * Check for a '_reflectionData' in the array to build the Reflection
		 */
		if (phalcon_array_isset_string(data, SS("_reflectionData"))) {
			PHALCON_OBS_VAR(reflection_data);
			phalcon_array_fetch_string(&reflection_data, data, SL("_reflectionData"), PH_NOISY);
			object_init_ex(return_value, phalcon_annotations_reflection_ce);
			PHALCON_CALL_METHOD(NULL, return_value, "__construct", reflection_data);
	
			RETURN_MM();
		}
	}
	object_init_ex(return_value, phalcon_annotations_reflection_ce);
	PHALCON_CALL_METHOD(NULL, return_value, "__construct");
	
	RETURN_MM();
}

