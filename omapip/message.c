/* message.c

   Subroutines for dealing with message objects. */

/*
 * Copyright (c) 1996-1999 Internet Software Consortium.
 * Use is subject to license terms which appear in the file named
 * ISC-LICENSE that should have accompanied this file when you
 * received it.   If a file named ISC-LICENSE did not accompany this
 * file, or you are not sure the one you have is correct, you may
 * obtain an applicable copy of the license at:
 *
 *             http://www.isc.org/isc-license-1.0.html. 
 *
 * This file is part of the ISC DHCP distribution.   The documentation
 * associated with this file is listed in the file DOCUMENTATION,
 * included in the top-level directory of this release.
 *
 * Support and other services are available for ISC products - see
 * http://www.isc.org for more information.
 */

#include <omapip/omapip.h>

omapi_message_object_t *omapi_registered_messages;

isc_result_t omapi_message_new (omapi_object_t **o, char *name)
{
	omapi_message_object_t *m;
	omapi_object_t *g;
	isc_result_t status;

	m = malloc (sizeof *m);
	if (!m)
		return ISC_R_NOMEMORY;
	memset (m, 0, sizeof *m);
	m -> type = omapi_type_message;
	m -> refcnt = 1;

	g = (omapi_object_t *)0;
	status = omapi_generic_new (&g, name);
	if (status != ISC_R_SUCCESS) {
		free (m);
		return status;
	}
	status = omapi_object_reference (&m -> inner, g, name);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&m, name);
		omapi_object_dereference (&g, name);
		return status;
	}
	status = omapi_object_reference (&g -> outer,
					 (omapi_object_t *)m, name);

	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&m, name);
		omapi_object_dereference (&g, name);
		return status;
	}

	status = omapi_object_reference (o, (omapi_object_t *)m, name);
	omapi_object_dereference ((omapi_object_t **)&m, name);
	omapi_object_dereference (&g, name);
	if (status != ISC_R_SUCCESS)
		return status;

	return status;
}

isc_result_t omapi_message_set_value (omapi_object_t *h,
				      omapi_object_t *id,
				      omapi_data_string_t *name,
				      omapi_typed_data_t *value)
{
	omapi_message_object_t *m;
	isc_result_t status;

	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;

	/* Can't set authlen. */

	/* Can set authenticator, but the value must be typed data. */
	if (!omapi_ds_strcmp (name, "authenticator")) {
		if (m -> authenticator)
			omapi_typed_data_dereference
				(&m -> authenticator,
				 "omapi_message_set_value");
		omapi_typed_data_reference (&m -> authenticator,
					    value,
					    "omapi_message_set_value");
		return ISC_R_SUCCESS;

	} else if (!omapi_ds_strcmp (name, "object")) {
		if (value -> type != omapi_datatype_object)
			return ISC_R_INVALIDARG;
		if (m -> object)
			omapi_object_dereference
				(&m -> object,
				 "omapi_message_set_value");
		omapi_object_reference (&m -> object,
					value -> u.object,
					"omapi_message_set_value");
		return ISC_R_SUCCESS;

	/* Can set authid, but it has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "authid")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> authid = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Can set op, but it has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "op")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> op = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Handle also has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "handle")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> h = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Transaction ID has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "id")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> id = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Remote transaction ID has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "rid")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> rid = value -> u.integer;
		return ISC_R_SUCCESS;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_message_get_value (omapi_object_t *h,
				      omapi_object_t *id,
				      omapi_data_string_t *name,
				      omapi_value_t **value)
{
	omapi_message_object_t *m;
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;

	/* Look for values that are in the message data structure. */
	if (!omapi_ds_strcmp (name, "authlen"))
		return omapi_make_int_value (value, name, m -> authlen,
					     "omapi_message_get_value");
	else if (!omapi_ds_strcmp (name, "authenticator")) {
		if (m -> authenticator)
			return omapi_make_value (value,
						 name, m -> authenticator,
						 "omapi_message_get_value");
		else
			return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "authid")) {
		return omapi_make_int_value (value, name, m -> authid,
					     "omapi_message_get_value");
	} else if (!omapi_ds_strcmp (name, "op")) {
		return omapi_make_int_value (value, name, m -> op,
					     "omapi_message_get_value");
	} else if (!omapi_ds_strcmp (name, "handle")) {
		return omapi_make_int_value (value, name, m -> handle,
					     "omapi_message_get_value");
	} else if (!omapi_ds_strcmp (name, "id")) {
		return omapi_make_int_value (value, name, m -> id, 
					     "omapi_message_get_value");
	} else if (!omapi_ds_strcmp (name, "rid")) {
		return omapi_make_int_value (value, name, m -> rid,
					     "omapi_message_get_value");
	}

	/* See if there's an inner object that has the value. */
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_message_destroy (omapi_object_t *h, char *name)
{
	int i;

	omapi_message_object_t *m;
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	if (m -> authenticator) {
		omapi_typed_data_dereference (&m -> authenticator, name);
	}
	if (!m -> prev && omapi_registered_messages != m)
		omapi_message_unregister (h);
	if (m -> prev)
		omapi_object_dereference ((omapi_object_t **)&m -> prev, name);
	if (m -> next)
		omapi_object_dereference ((omapi_object_t **)&m -> next, name);
	if (m -> id_object)
		omapi_object_dereference ((omapi_object_t **)&m -> id_object,
					  name);
	if (m -> object)
		omapi_object_dereference ((omapi_object_t **)&m -> object,
					  name);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_signal_handler (omapi_object_t *h,
					   char *name, va_list ap)
{
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t omapi_message_stuff_values (omapi_object_t *c,
					 omapi_object_t *id,
					 omapi_object_t *m)
{
	int i;

	if (m -> type != omapi_type_message)
		return ISC_R_INVALIDARG;

	if (m -> inner && m -> inner -> type -> stuff_values)
		return (*(m -> inner -> type -> stuff_values)) (c, id,
								m -> inner);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_register (omapi_object_t *mo)
{
	omapi_message_object_t *m;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)mo;
	
	/* Already registered? */
	if (m -> prev || m -> next || omapi_registered_messages == m)
		return ISC_R_INVALIDARG;

	if (omapi_registered_messages) {
		omapi_object_reference
			((omapi_object_t **)&m -> next,
			 (omapi_object_t *)omapi_registered_messages,
			 "omapi_message_register");
		omapi_object_reference
			((omapi_object_t **)&omapi_registered_messages -> prev,
			 (omapi_object_t *)m, "omapi_message_register");
		omapi_object_dereference
			((omapi_object_t **)&omapi_registered_messages,
			 "omapi_message_register");
	}
	omapi_object_reference
		((omapi_object_t **)&omapi_registered_messages,
		 (omapi_object_t *)m, "omapi_message_register");
	return ISC_R_SUCCESS;;
}

isc_result_t omapi_message_unregister (omapi_object_t *mo)
{
	omapi_message_object_t *m;
	omapi_message_object_t *n;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)mo;
	
	/* Not registered? */
	if (!m -> prev && omapi_registered_messages != m)
		return ISC_R_INVALIDARG;

	n = (omapi_message_object_t *)0;
	if (m -> next) {
		omapi_object_reference ((omapi_object_t **)&n,
					(omapi_object_t *)m -> next,
					"omapi_message_unregister");
		omapi_object_dereference ((omapi_object_t **)&m -> next,
					  "omapi_message_unregister");
	}
	if (m -> prev) {
		omapi_message_object_t *tmp = (omapi_message_object_t *)0;
		omapi_object_reference ((omapi_object_t **)&tmp,
					(omapi_object_t *)m -> prev,
					"omapi_message_register");
		omapi_object_dereference ((omapi_object_t **)&m -> prev,
					  "omapi_message_unregister");
		if (tmp -> next)
			omapi_object_dereference
				((omapi_object_t **)&tmp -> next,
				 "omapi_message_unregister");
		if (n)
			omapi_object_reference
				((omapi_object_t **)&tmp -> next,
				 (omapi_object_t *)n,
				 "omapi_message_unregister");
		omapi_object_dereference ((omapi_object_t **)&tmp,
					  "omapi_message_unregister");
	} else {
		omapi_object_dereference
			((omapi_object_t **)&omapi_registered_messages,
			 "omapi_unregister_message");
		if (n)
			omapi_object_reference
				((omapi_object_t **)&omapi_registered_messages,
				 (omapi_object_t *)n,
				 "omapi_message_unregister");
	}
	if (n)
		omapi_object_dereference ((omapi_object_t **)&n,
					  "omapi_message_unregister");
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_process (omapi_object_t *mo, omapi_object_t *po)
{
	omapi_message_object_t *message, *m;
	omapi_object_t *object = (omapi_object_t *)0;
	omapi_value_t *tv;
	int create, update, exclusive;
	isc_result_t status, waitstatus;
	omapi_object_type_t *type;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	message = (omapi_message_object_t *)mo;

	if (message -> rid) {
		for (m = omapi_registered_messages; m; m = m -> next)
			if (m -> id == message -> rid)
				break;
		/* If we don't have a real message corresponding to
		   the message ID to which this message claims it is a
		   response, something's fishy. */
		if (!m)
			return ISC_R_NOTFOUND;
	}

	switch (message -> op) {
	      case OMAPI_OP_OPEN:
		if (m) {
			omapi_protocol_send_error (po, ISC_R_INVALIDARG,
						   message -> id,
						   "OPEN can't be a response");
			return ISC_R_SUCCESS;
		}

		/* Get the type of the requested object, if one was
		   specified. */
		tv = (omapi_value_t *)0;
		status = omapi_get_value_str (message -> object,
					      (omapi_object_t *)0,
					      "type", &tv);
		if (status == ISC_R_SUCCESS &&
		    (tv -> value -> type == omapi_datatype_data ||
		     tv -> value -> type == omapi_datatype_string)) {
			for (type = omapi_object_types;
			     type; type = type -> next)
				if (!omapi_td_strcmp (tv -> value,
						      type -> name))
					break;
		} else
			type = (omapi_object_type_t *)0;
		if (tv)
			omapi_value_dereference (&tv,
						 "omapi_message_process");

		/* Get the create flag. */
		status = omapi_get_value_str (message -> object,
					      (omapi_object_t *)0,
					      "create", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&create, tv);
			omapi_value_dereference (&tv,
						      "omapi_message_process");
			if (status != ISC_R_SUCCESS) {
				omapi_protocol_send_error
					(po, status, message -> id,
					 "invalid create flag value");
				return ISC_R_SUCCESS;
			}
		} else
			create = 0;

		/* Get the update flag. */
		status = omapi_get_value_str (message -> object,
					      (omapi_object_t *)0,
					      "update", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&update, tv);
			omapi_value_dereference (&tv,
						      "omapi_message_process");
			if (status != ISC_R_SUCCESS) {
				omapi_protocol_send_error
					(po, status, message -> id,
					 "invalid update flag value");
				return ISC_R_SUCCESS;
			}
		} else
			update = 0;

		/* Get the exclusive flag. */
		status = omapi_get_value_str (message -> object,
					      (omapi_object_t *)0,
					      "exclusive", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&exclusive, tv);
			omapi_value_dereference (&tv,
						 "omapi_message_process");
			if (status != ISC_R_SUCCESS) {
				omapi_protocol_send_error
					(po, status, message -> id,
					 "invalid exclusive flag value");
				return ISC_R_SUCCESS;
			}
		} else
			exclusive = 0;

		/* If we weren't given a type, look the object up with
                   the handle. */
		if (!type) {
			if (create) {
				omapi_protocol_send_error
					(po, ISC_R_INVALIDARG, message -> id,
					 "type required on create");
				return ISC_R_SUCCESS;
			}
			goto refresh;
		}

		/* If the type doesn't provide a lookup method, we can't
		   look up the object. */
		if (!type -> lookup) {
			omapi_protocol_send_error
				(po, ISC_R_NOTIMPLEMENTED, message -> id,
				 "unsearchable object type");
			return ISC_R_SUCCESS;
		}
		status = (*(type -> lookup)) (&object, (omapi_object_t *)0,
					      message -> object);

		if (status != ISC_R_SUCCESS && status != ISC_R_NOTFOUND) {
			omapi_protocol_send_error
				(po, status, message -> id,
				 "object lookup failed");
			return ISC_R_SUCCESS;
		}

		/* If we didn't find the object and we aren't supposed to
		   create it, return an error. */
		if (status == ISC_R_NOTFOUND && !create) {
			omapi_protocol_send_error
				(po, ISC_R_NOTFOUND, message -> id,
				 "no object matches specification");
			return ISC_R_SUCCESS;
		}			

		/* If we found an object, we're supposed to be creating an
		   object, and we're not supposed to have found an object,
		   return an error. */
		if (status == ISC_R_SUCCESS && create && exclusive) {
			omapi_object_dereference
				(&object, "omapi_message_process");
			omapi_protocol_send_error
				(po, ISC_R_EXISTS, message -> id,
				 "specified object already exists");
			return ISC_R_SUCCESS;
		}

		/* If we're creating the object, do it now. */
		if (!object) {
			status = omapi_object_create (&object, type);
			if (status != ISC_R_SUCCESS) {
				omapi_protocol_send_error
					(po, status, message -> id,
					 "can't create new object");
				return ISC_R_SUCCESS;
			}
		}

		/* If we're updating it, do so now. */
		if (create || update) {
			status = omapi_object_update (object,
						      message -> object);
			if (status != ISC_R_SUCCESS) {
				omapi_object_dereference
					(&object, "omapi_message_process");
				omapi_protocol_send_error
					(po, status, message -> id,
					 "can't update object");
				return ISC_R_SUCCESS;
			}
		}
		
		/* Now send the new contents of the object back in
		   response. */
		goto send;

	      case OMAPI_OP_REFRESH:
	      refresh:
		status = omapi_handle_lookup (&object,
					      message -> handle);
		if (status != ISC_R_SUCCESS) {
			omapi_protocol_send_error
				(po, status, message -> id,
				 "no matching handle");
			return ISC_R_SUCCESS;
		}
	      send:		
		omapi_protocol_send_update (po, message -> id, object);
		omapi_object_dereference (&object,
					  "omapi_message_process");
		return ISC_R_SUCCESS;

	      case OMAPI_OP_UPDATE:
		if (m) {
			omapi_object_reference (&object, m -> object,
						"omapi_message_process");
		} else {
			status = omapi_handle_lookup (&object,
						      message -> handle);
			if (status != ISC_R_SUCCESS) {
				omapi_protocol_send_error
					(po, status, message -> id,
					 "no matching handle");
				return ISC_R_SUCCESS;
			}
		}

		status = omapi_object_update (object, message -> object);
		if (status != ISC_R_SUCCESS) {
			omapi_object_dereference
				(&object, "omapi_message_process");
			if (!message -> rid)
				omapi_protocol_send_error
					(po, status, message -> id,
					 "can't update object");
			if (m)
				omapi_signal ((omapi_object_t *)m,
					      "status", status);
			return ISC_R_SUCCESS;
		}
		if (!message -> rid)
			omapi_protocol_send_success (po, message -> id);
		if (m)
			omapi_signal ((omapi_object_t *)m,
				      "status", ISC_R_SUCCESS);
		return ISC_R_SUCCESS;

	      case OMAPI_OP_NOTIFY:
		omapi_protocol_send_error (po, ISC_R_NOTIMPLEMENTED,
					   message -> id,
					   "notify not implemented yet");
		return ISC_R_SUCCESS;

	      case OMAPI_OP_ERROR:
		/* An error message that's not in response to another
		   message is invalid. */
		if (!m)
			return ISC_R_UNEXPECTED;

		/* Get the wait status. */
		status = omapi_get_value_str (message -> object,
					      (omapi_object_t *)0,
					      "result", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&waitstatus, tv);
			omapi_value_dereference (&tv,
						 "omapi_message_process");
			if (status != ISC_R_SUCCESS)
				waitstatus = ISC_R_UNEXPECTED;
		} else
			waitstatus = ISC_R_UNEXPECTED;
		omapi_signal ((omapi_object_t *)m, "status", waitstatus);
		return ISC_R_SUCCESS;

	      case OMAPI_OP_REQUEST_OK:
		/* An error message that's not in response to another
		   message is invalid. */
		if (!m)
			return ISC_R_UNEXPECTED;

		omapi_signal ((omapi_object_t *)m, "status", ISC_R_SUCCESS);
		return ISC_R_SUCCESS;
	}
	return ISC_R_NOTIMPLEMENTED;
}
