/**************************************************************************
*
* Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
* Contributions by Nikola Jelic 2011 <nikola.jelic@euroicc.com>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stdint.h>
#include "bacnet.h"
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "wp.h"
#include "bacnet.h"

/** @file wpm.c  Encode/Decode BACnet Write Property Multiple APDUs  */

/** Decoding for WritePropertyMultiple service, object ID.
 * @ingroup DSWPM
 * This handler will be invoked by write_property_multiple handler
 * if it has been enabled by a call to apdu_set_confirmed_handler().
 * This function decodes only the first tagged entity, which is
 * an object identifier.  This function will return an error if:
 *   - the tag is not the right value
 *   - the number of bytes is not enough to decode for this entity
 *   - the subsequent tag number is incorrect
 *
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_len [in] The length of the APDU buffer.
 * @param data [out] The BACNET_WRITE_PROPERTY_DATA structure
 *    which will contain the reponse values or error.
 */
int wpm_decode_object_id(
    uint8_t * apdu,
    uint16_t apdu_len,
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t object_instance = 0;
    uint16_t object_type = 0;
    uint16_t len = 0;

    if (apdu && (apdu_len > 5) && wp_data) {
        /* Context tag 0 - Object ID */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if ((tag_number == 0) && (apdu_len > len)) {
            apdu_len -= len;
            if (apdu_len >= 4) {
                len +=
                    decode_object_id(&apdu[len], &object_type,
                    &object_instance);

                wp_data->object_type = object_type;
                wp_data->object_instance = object_instance;
                apdu_len -= len;
            } else {
                wp_data->error_code =
                    ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                return BACNET_STATUS_REJECT;
            }
        } else {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* just test for the next tag - no need to decode it here */
        /* Context tag 1: sequence of BACnetPropertyValue */
        if (apdu_len && !decode_is_opening_tag_number(&apdu[len], 1)) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
    } else {
        wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        return BACNET_STATUS_REJECT;
    }

    return (int) len;
}


int wpm_decode_object_property(
    uint8_t * apdu,
    uint16_t apdu_len,
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t ulVal = 0;
    int len = 0, i = 0;


    if ((apdu) && (apdu_len) && (wp_data)) {
        wp_data->array_index = BACNET_ARRAY_ALL;
        wp_data->priority = BACNET_MAX_PRIORITY;
        wp_data->application_data_len = 0;
        /* tag 0 - Property Identifier */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == 0) {
            len += decode_enumerated(&apdu[len], len_value, &ulVal);
            wp_data->object_property = ulVal;
        } else {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        /* tag 1 - Property Array Index - optional */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == 1) {
            len += decode_unsigned(&apdu[len], len_value, &ulVal);
            wp_data->array_index = ulVal;

            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
        }
        /* tag 2 - Property Value */
        if ((tag_number == 2) && (decode_is_opening_tag(&apdu[len - 1]))) {
            len--;
            wp_data->application_data_len =
                bacapp_data_len(&apdu[len], (unsigned) (apdu_len - len),
                wp_data->object_property);
            len++;

            /* copy application data */
            for (i = 0; i < wp_data->application_data_len; i++) {
                wp_data->application_data[i] = apdu[len + i];
            }
            len += wp_data->application_data_len;
            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
            /* closing tag 2 */
            if ((tag_number != 2) && (decode_is_closing_tag(&apdu[len - 1]))) {
                wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
                return BACNET_STATUS_REJECT;
            }
        } else {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        /* tag 3 - Priority - optional */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == 3) {
            len += decode_unsigned(&apdu[len], len_value, &ulVal);
            wp_data->priority = ulVal;
        } else
            len--;
    } else {
        wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        return BACNET_STATUS_REJECT;
    }

    return len;
}

/* encode functions */
int wpm_encode_apdu_init(
    uint8_t * apdu,
    uint8_t invoke_id)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;        /* service choice */
        apdu_len = 4;
    }

    return apdu_len;

}

int wpm_encode_apdu_object_begin(
    uint8_t * apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu_len =
            encode_context_object_id(&apdu[0], 0, object_type,
            object_instance);
        /* Tag 1: sequence of WriteAccessSpecification */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
    }

    return apdu_len;
}

int wpm_encode_apdu_object_end(
    uint8_t * apdu)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 1);
    }

    return apdu_len;
}

int wpm_encode_apdu_object_property(
    uint8_t * apdu,
    BACNET_WRITE_PROPERTY_DATA * wpdata)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu_len =
            encode_context_enumerated(&apdu[0], 0, wpdata->object_property);
        /* optional array index */
        if (wpdata->array_index != BACNET_ARRAY_ALL) {
            apdu_len +=
                encode_context_unsigned(&apdu[apdu_len], 1,
                wpdata->array_index);
        }
        apdu_len += encode_opening_tag(&apdu[apdu_len], 2);
        for (len = 0; len < wpdata->application_data_len; len++) {
            apdu[apdu_len] = wpdata->application_data[len];
            apdu_len++;
        }
        apdu_len += encode_closing_tag(&apdu[apdu_len], 2);
        if (wpdata->priority != BACNET_NO_PRIORITY) {
            encode_context_unsigned(&apdu[apdu_len], 3, wpdata->priority);
        }
    }

    return apdu_len;
}

int wpm_ack_encode_apdu_init(
    uint8_t * apdu,
    uint8_t invoke_id)
{
    int len = 0;

    if (apdu) {
        apdu[len++] = PDU_TYPE_SIMPLE_ACK;
        apdu[len++] = invoke_id;
        apdu[len++] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;
    }

    return len;
}

int wpm_error_ack_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    int len = 0;

    if (apdu) {
        apdu[len++] = PDU_TYPE_ERROR;
        apdu[len++] = invoke_id;
        apdu[len++] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;

        len += encode_opening_tag(&apdu[len], 0);
        len += encode_application_enumerated(&apdu[len], wp_data->error_class);
        len += encode_application_enumerated(&apdu[len], wp_data->error_code);
        len += encode_closing_tag(&apdu[len], 0);

        len += encode_opening_tag(&apdu[len], 1);
        len +=
            encode_context_object_id(&apdu[len], 0, wp_data->object_type,
            wp_data->object_instance);
        len +=
            encode_context_enumerated(&apdu[len], 1, wp_data->object_property);

        if (wp_data->array_index != BACNET_ARRAY_ALL)
            len +=
                encode_context_unsigned(&apdu[len], 2, wp_data->array_index);
        len += encode_closing_tag(&apdu[len], 1);
    }
    return len;
}
