/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include "bacenum.h"
#include "bacnet.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "arf.h"

/** @file arf.c  Atomic Read File */
#if BAC_FILE
/* encode service */
int arf_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_ATOMIC_READ_FILE;   /* service choice */
        apdu_len = 4;
        apdu_len +=
            encode_application_object_id(&apdu[apdu_len], data->object_type,
            data->object_instance);
        switch (data->access) {
            case FILE_STREAM_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                apdu_len +=
                    encode_application_signed(&apdu[apdu_len],
                    data->type.stream.fileStartPosition);
                apdu_len +=
                    encode_application_unsigned(&apdu[apdu_len],
                    data->type.stream.requestedOctetCount);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                break;
            case FILE_RECORD_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
                apdu_len +=
                    encode_application_signed(&apdu[apdu_len],
                    data->type.record.fileStartRecord);
                apdu_len +=
                    encode_application_unsigned(&apdu[apdu_len],
                    data->type.record.RecordCount);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 1);
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

/* decode the service request only */
int arf_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint16_t type = 0;  /* for decoding */

    /* check for value pointers */
    if (apdu_len && data) {
        len =
            decode_tag_number_and_value(&apdu[0], &tag_number,
            &len_value_type);
        if (tag_number != BACNET_APPLICATION_TAG_OBJECT_ID)
            return -1;
        len += decode_object_id(&apdu[len], &type, &data->object_instance);
        data->object_type = (BACNET_OBJECT_TYPE) type;
        if (decode_is_opening_tag_number(&apdu[len], 0)) {
            data->access = FILE_STREAM_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartPosition */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT)
                return -1;
            len +=
                decode_signed(&apdu[len], len_value_type,
                &data->type.stream.fileStartPosition);
            /* requestedOctetCount */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT)
                return -1;
            len +=
                decode_unsigned(&apdu[len], len_value_type,
                &data->type.stream.requestedOctetCount);
            if (!decode_is_closing_tag_number(&apdu[len], 0))
                return -1;
            /* a tag number is not extended so only one octet */
            len++;
        } else if (decode_is_opening_tag_number(&apdu[len], 1)) {
            data->access = FILE_RECORD_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartRecord */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT)
                return -1;
            len +=
                decode_signed(&apdu[len], len_value_type,
                &data->type.record.fileStartRecord);
            /* RecordCount */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT)
                return -1;
            len +=
                decode_unsigned(&apdu[len], len_value_type,
                &data->type.record.RecordCount);
            if (!decode_is_closing_tag_number(&apdu[len], 1))
                return -1;
            /* a tag number is not extended so only one octet */
            len++;
        } else
            return -1;
    }

    return len;
}

int arf_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    uint8_t * invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2];       /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_ATOMIC_READ_FILE)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len =
            arf_decode_service_request(&apdu[offset], apdu_len - offset, data);
    }

    return len;
}

/* encode service */
int arf_ack_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_ATOMIC_READ_FILE;   /* service choice */
        apdu_len = 3;
        /* endOfFile */
        apdu_len +=
            encode_application_boolean(&apdu[apdu_len], data->endOfFile);
        switch (data->access) {
            case FILE_STREAM_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                apdu_len +=
                    encode_application_signed(&apdu[apdu_len],
                    data->type.stream.fileStartPosition);
                apdu_len +=
                    encode_application_octet_string(&apdu[apdu_len],
                    &data->fileData);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                break;
            case FILE_RECORD_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
                apdu_len +=
                    encode_application_signed(&apdu[apdu_len],
                    data->type.record.fileStartRecord);
                apdu_len +=
                    encode_application_unsigned(&apdu[apdu_len],
                    data->type.record.RecordCount);
                apdu_len +=
                    encode_application_octet_string(&apdu[apdu_len],
                    &data->fileData);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 1);
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

/* decode the service request only */
int arf_ack_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int len = 0;
    int tag_len = 0;
    int decoded_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    /* check for value pointers */
    if (apdu_len && data) {
        len =
            decode_tag_number_and_value(&apdu[0], &tag_number,
            &len_value_type);
        if (tag_number != BACNET_APPLICATION_TAG_BOOLEAN) {
            return -1;
        }
        data->endOfFile = decode_boolean(len_value_type);
        if (decode_is_opening_tag_number(&apdu[len], 0)) {
            data->access = FILE_STREAM_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartPosition */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT) {
                return -1;
            }
            len +=
                decode_signed(&apdu[len], len_value_type,
                &data->type.stream.fileStartPosition);
            /* fileData */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING) {
                return -1;
            }
            decoded_len =
                decode_octet_string(&apdu[len], len_value_type,
                &data->fileData);
            if (decoded_len != len_value_type) {
                return -1;
            }
            len += decoded_len;
            if (!decode_is_closing_tag_number(&apdu[len], 0)) {
                return -1;
            }
            /* a tag number is not extended so only one octet */
            len++;
        } else if (decode_is_opening_tag_number(&apdu[len], 1)) {
            data->access = FILE_RECORD_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartRecord */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT) {
                return -1;
            }
            len +=
                decode_signed(&apdu[len], len_value_type,
                &data->type.record.fileStartRecord);
            /* returnedRecordCount */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                return -1;
            }
            len +=
                decode_unsigned(&apdu[len], len_value_type,
                &data->type.record.RecordCount);
            /* fileData */
            tag_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING)
                return -1;
            len +=
                decode_octet_string(&apdu[len], len_value_type,
                &data->fileData);
            if (!decode_is_closing_tag_number(&apdu[len], 1)) {
                return -1;
            }
            /* a tag number is not extended so only one octet */
            len++;
        } else {
            return -1;
        }
    }

    return len;
}

int arf_ack_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    uint8_t * invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA * data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];       /* invoke id - filled in by net layer */
    if (apdu[2] != SERVICE_CONFIRMED_ATOMIC_READ_FILE)
        return -1;
    offset = 3;

    if (apdu_len > offset) {
        len =
            arf_ack_decode_service_request(&apdu[offset], apdu_len - offset,
            data);
    }

    return len;
}
#endif


