
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

#include "validation/validator/exclusionin.h"
#include "validation/validator.h"
#include "validation/validatorinterface.h"
#include "validation/message.h"
#include "validation/exception.h"
#include "validation.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/exception.h"
#include "kernel/array.h"
#include "kernel/string.h"
#include "kernel/concat.h"
#include "kernel/operators.h"

#include "interned-strings.h"

/**
 * Phalcon\Validation\Validator\ExclusionIn
 *
 * Check if a value is not included into a list of values
 *
 *<code>
 *use Phalcon\Validation\Validator\ExclusionIn;
 *
 *$validator->add('status', new ExclusionIn(array(
 *   'message' => 'The status must not be A or B',
 *   'domain' => array('A', 'B')
 *)));
 *</code>
 */
zend_class_entry *phalcon_validation_validator_exclusionin_ce;

PHP_METHOD(Phalcon_Validation_Validator_ExclusionIn, validate);
PHP_METHOD(Phalcon_Validation_Validator_ExclusionIn, valid);

static const zend_function_entry phalcon_validation_validator_exclusionin_method_entry[] = {
	PHP_ME(Phalcon_Validation_Validator_ExclusionIn, validate, arginfo_phalcon_validation_validatorinterface_validate, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation_Validator_ExclusionIn, valid, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Validation\Validator\ExclusionIn initializer
 */
PHALCON_INIT_CLASS(Phalcon_Validation_Validator_ExclusionIn){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Validation\\Validator, ExclusionIn, validation_validator_exclusionin, phalcon_validation_validator_ce, phalcon_validation_validator_exclusionin_method_entry, 0);

	zend_class_implements(phalcon_validation_validator_exclusionin_ce, 1, phalcon_validation_validatorinterface_ce);

	return SUCCESS;
}

/**
 * Executes the validation
 *
 * @param Phalcon\Validation $validator
 * @param string $attribute
 * @return boolean
 */
PHP_METHOD(Phalcon_Validation_Validator_ExclusionIn, validate){

	zval *validator, *attribute, *value = NULL, *allow_empty, *domain, *valid = NULL;
	zval *label, *joined_domain, *pairs;
	zval *message_str, *code, *prepared = NULL, *message;
	zend_class_entry *ce = Z_OBJCE_P(getThis());

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &validator, &attribute);
	
	PHALCON_VERIFY_CLASS_EX(validator, phalcon_validation_ce, phalcon_validation_exception_ce, 1);

	PHALCON_CALL_METHOD(&value, validator, "getvalue", attribute);

	PHALCON_OBS_VAR(allow_empty);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &allow_empty, getThis(), ISV(allowEmpty)));
	if (zend_is_true(allow_empty) && phalcon_validation_validator_isempty_helper(value)) {
		RETURN_MM_TRUE;
	}

	/* A domain is an array with a list of valid values */
	PHALCON_OBS_VAR(domain);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &domain, getThis(), ISV(domain)));
	if (Z_TYPE_P(domain) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "Option 'domain' must be an array");
		return;
	}

	PHALCON_CALL_SELF(&valid, "valid", value, domain);
	
	if (PHALCON_IS_FALSE(valid)) {
		PHALCON_OBS_VAR(label);
		RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &label, getThis(), ISV(label)));
		if (!zend_is_true(label)) {
			PHALCON_CALL_METHOD(&label, validator, "getlabel", attribute);
			if (!zend_is_true(label)) {
				PHALCON_CPY_WRT(label, attribute);
			}
		}

		ALLOC_INIT_ZVAL(joined_domain);
		phalcon_fast_join_str(joined_domain, SL(", "), domain);

		PHALCON_ALLOC_GHOST_ZVAL(pairs);
		array_init_size(pairs, 2);
		Z_ADDREF_P(label); add_assoc_zval_ex(pairs, SS(":field"), label);
		add_assoc_zval_ex(pairs, SS(":domain"), joined_domain);

		PHALCON_OBS_VAR(message_str);
		RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
		if (!zend_is_true(message_str)) {
			PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "ExclusionIn"));
		}
	
		PHALCON_OBS_VAR(code);
		RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &code, getThis(), ISV(code)));
		if (Z_TYPE_P(code) == IS_NULL) {
			ZVAL_LONG(code, 0);
		}

		PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);
	
		message = phalcon_validation_message_construct_helper(prepared, attribute, "ExclusionIn", code);
		Z_DELREF_P(message);
	
		PHALCON_CALL_METHOD(NULL, validator, "appendmessage", message);
		RETURN_MM_FALSE;
	}
	
	RETURN_MM_TRUE;
}

/**
 * Executes the validation
 *
 * @param string $value
 * @return boolean
 */
PHP_METHOD(Phalcon_Validation_Validator_ExclusionIn, valid){

	zval *value, *domain;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &value, &domain);
	
	/** 
	 * Check if the value is contained by the array
	 */
	if (phalcon_fast_in_array(value, domain)) {
		RETURN_MM_FALSE;
	}
	
	RETURN_MM_TRUE;
}
