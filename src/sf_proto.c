/*
 * Copyright (c) 2020 YuQing <384681@qq.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#include <errno.h>
#include "fastcommon/shared_func.h"
#include "sf_util.h"
#include "sf_proto.h"

int sf_proto_set_body_length(struct fast_task_info *task)
{
    SFCommonProtoHeader *header;

    header = (SFCommonProtoHeader *)task->data;
    if (!SF_PROTO_CHECK_MAGIC(header->magic)) {
        logError("file: "__FILE__", line: %d, "
                "peer %s:%u, magic "SF_PROTO_MAGIC_FORMAT
                " is invalid, expect: "SF_PROTO_MAGIC_FORMAT,
                __LINE__, task->client_ip, task->port,
                SF_PROTO_MAGIC_PARAMS(header->magic),
                SF_PROTO_MAGIC_EXPECT_PARAMS);
        return EINVAL;
    }

    task->length = buff2int(header->body_len); //set body length
    return 0;
}

int sf_check_response(ConnectionInfo *conn, SFResponseInfo *response,
        const int network_timeout, const unsigned char expect_cmd)
{
    int result;

    if (response->header.status == 0) {
        if (response->header.cmd != expect_cmd) {
            response->error.length = sprintf(
                    response->error.message,
                    "response cmd: %d != expect: %d",
                    response->header.cmd, expect_cmd);
            return EINVAL;
        }

        return 0;
    }

    if (response->header.body_len > 0) {
        int recv_bytes;
        if (response->header.body_len >= sizeof(response->error.message)) {
            response->error.length = sizeof(response->error.message) - 1;
        } else {
            response->error.length = response->header.body_len;
        }

        if ((result=tcprecvdata_nb_ex(conn->sock, response->error.message,
                response->error.length, network_timeout, &recv_bytes)) == 0)
        {
            response->error.message[response->error.length] = '\0';
        } else {
            response->error.length = snprintf(response->error.message,
                    sizeof(response->error.message),
                    "recv error message fail, "
                    "recv bytes: %d, expect message length: %d, "
                    "errno: %d, error info: %s", recv_bytes,
                    response->error.length, result, STRERROR(result));
        }
    } else {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message), "response status %d, "
                "error info: %s", response->header.status,
                sf_strerror(response->header.status));
    }

    return response->header.status;
}

int sf_send_and_recv_response_header(ConnectionInfo *conn, char *data,
        const int len, SFResponseInfo *response, const int network_timeout)
{
    int result;
    SFCommonProtoHeader header_proto;

    if ((result=tcpsenddata_nb(conn->sock, data, len, network_timeout)) != 0) {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "send data fail, errno: %d, error info: %s",
                result, STRERROR(result));
        return result;
    }

    if ((result=tcprecvdata_nb(conn->sock, &header_proto,
            sizeof(SFCommonProtoHeader), network_timeout)) != 0)
    {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "recv data fail, errno: %d, error info: %s",
                result, STRERROR(result));
        return result;
    }

    sf_proto_extract_header(&header_proto, &response->header);
    return 0;
}

int sf_send_and_recv_response_ex(ConnectionInfo *conn, char *send_data,
        const int send_len, SFResponseInfo *response,
        const int network_timeout, const unsigned char expect_cmd,
        char *recv_data, const int *expect_body_lens,
        const int expect_body_len_count, int *body_len)
{
    int result;
    int recv_bytes;
    int i;

    if ((result=sf_send_and_check_response_header(conn, send_data, send_len,
                    response, network_timeout, expect_cmd)) != 0)
    {
        return result;
    }

    if (body_len != NULL) {
        *body_len = response->header.body_len;
    }

    if (response->header.body_len != expect_body_lens[0]) {
        for (i=1; i<expect_body_len_count; i++) {
            if (response->header.body_len == expect_body_lens[i]) {
                break;
            }
        }

        if (i == expect_body_len_count) {
            if (expect_body_len_count == 1) {
                response->error.length = sprintf(
                        response->error.message,
                        "response body length: %d != %d",
                        response->header.body_len,
                        expect_body_lens[0]);
            } else {
                response->error.length = sprintf(
                        response->error.message,
                        "response body length: %d not in [%d",
                        response->header.body_len,
                        expect_body_lens[0]);
                for (i=1; i<expect_body_len_count; i++) {
                    response->error.length += sprintf(
                            response->error.message +
                            response->error.length,
                            ", %d", expect_body_lens[i]);
                }
                *(response->error.message + response->error.length++) = ']';
                *(response->error.message + response->error.length) = '\0';
            }
            return EINVAL;
        }
    }
    if (response->header.body_len == 0) {
        return 0;
    }

    if ((result=tcprecvdata_nb_ex(conn->sock, recv_data, response->
                    header.body_len, network_timeout, &recv_bytes)) != 0)
    {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "recv body fail, recv bytes: %d, expect body length: %d, "
                "errno: %d, error info: %s", recv_bytes,
                response->header.body_len,
                result, STRERROR(result));
    }
    return result;
}

int sf_send_and_recv_response_ex1(ConnectionInfo *conn, char *send_data,
        const int send_len, SFResponseInfo *response,
        const int network_timeout, const unsigned char expect_cmd,
        char *recv_data, const int buff_size, int *body_len)
{
    int result;

    if ((result=sf_send_and_check_response_header(conn, send_data, send_len,
                    response, network_timeout, expect_cmd)) != 0)
    {
        *body_len = 0;
        return result;
    }

    if (response->header.body_len == 0) {
        *body_len = 0;
        return 0;
    }

    if (response->header.body_len > buff_size) {
        response->error.length = sprintf(response->error.message,
                "response body length: %d exceeds buffer size: %d",
                response->header.body_len, buff_size);
        *body_len = 0;
        return EOVERFLOW;
    }

    if ((result=tcprecvdata_nb_ex(conn->sock, recv_data, response->
                    header.body_len, network_timeout, body_len)) != 0)
    {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "recv body fail, recv bytes: %d, expect body length: %d, "
                "errno: %d, error info: %s", *body_len, response->
                header.body_len, result, STRERROR(result));
    }
    return result;
}

int sf_recv_response(ConnectionInfo *conn, SFResponseInfo *response,
        const int network_timeout, const unsigned char expect_cmd,
        char *recv_data, const int expect_body_len)
{
    int result;
    int recv_bytes;
    SFCommonProtoHeader header_proto;

    if ((result=tcprecvdata_nb(conn->sock, &header_proto,
                    sizeof(SFCommonProtoHeader), network_timeout)) != 0)
    {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "recv data fail, errno: %d, error info: %s",
                result, STRERROR(result));
        return result;
    }
    sf_proto_extract_header(&header_proto, &response->header);

    if ((result=sf_check_response(conn, response, network_timeout,
                    expect_cmd)) != 0)
    {
        return result;
    }

    if (response->header.body_len != expect_body_len) {
        response->error.length = sprintf(response->error.message,
                "response body length: %d != %d",
                response->header.body_len,
                expect_body_len);
        return EINVAL;
    }
    if (expect_body_len == 0) {
        return 0;
    }

    if ((result=tcprecvdata_nb_ex(conn->sock, recv_data,
                    expect_body_len, network_timeout, &recv_bytes)) != 0)
    {
        response->error.length = snprintf(response->error.message,
                sizeof(response->error.message),
                "recv body fail, recv bytes: %d, expect body length: %d, "
                "errno: %d, error info: %s", recv_bytes,
                response->header.body_len,
                result, STRERROR(result));
    }

    return result;
}

const char *sf_get_cmd_caption(const int cmd)
{
    switch (cmd) {
        case SF_PROTO_ACK:
            return "ACK";
        case SF_PROTO_ACTIVE_TEST_REQ:
            return "ACTIVE_TEST_REQ";
        case SF_PROTO_ACTIVE_TEST_RESP:
            return "ACTIVE_TEST_RESP";
        case SF_SERVICE_PROTO_SETUP_CHANNEL_REQ:
            return "SETUP_CHANNEL_REQ";
        case SF_SERVICE_PROTO_SETUP_CHANNEL_RESP:
            return "SETUP_CHANNEL_RESP";
        case SF_SERVICE_PROTO_CLOSE_CHANNEL_REQ:
            return "CLOSE_CHANNEL_REQ";
        case SF_SERVICE_PROTO_CLOSE_CHANNEL_RESP:
            return "CLOSE_CHANNEL_RESP";
        case SF_SERVICE_PROTO_REBIND_CHANNEL_REQ:
            return "REBIND_CHANNEL_REQ";
        case SF_SERVICE_PROTO_REBIND_CHANNEL_RESP:
            return "REBIND_CHANNEL_RESP";
        case SF_SERVICE_PROTO_REPORT_REQ_RECEIPT_REQ:
            return "REPORT_REQ_RECEIPT_REQ";
        case SF_SERVICE_PROTO_REPORT_REQ_RECEIPT_RESP:
            return "REPORT_REQ_RECEIPT_RESP";
        default:
            return "UNKOWN";
    }
}

int sf_proto_deal_ack(struct fast_task_info *task,
        SFRequestInfo *request, SFResponseInfo *response)
{
    if (request->header.status != 0) {
        if (request->header.body_len > 0) {
            int remain_size;
            int len;

            response->error.length = sprintf(response->error.message,
                    "message from peer %s:%u => ",
                    task->client_ip, task->port);
            remain_size = sizeof(response->error.message) -
                response->error.length;
            if (request->header.body_len >= remain_size) {
                len = remain_size - 1;
            } else {
                len = request->header.body_len;
            }

            memcpy(response->error.message + response->error.length,
                    request->body, len);
            response->error.length += len;
            *(response->error.message + response->error.length) = '\0';
        }

        return request->header.status;
    }

    if (request->header.body_len > 0) {
        response->error.length = sprintf(response->error.message,
                "ACK body length: %d != 0", request->header.body_len);
        return -EINVAL;
    }

    return 0;
}

int sf_proto_rebind_idempotency_channel(ConnectionInfo *conn,
        const uint32_t channel_id, const int key, const int network_timeout)
{
    char out_buff[sizeof(SFCommonProtoHeader) +
        sizeof(SFProtoRebindChannelReq)];
    SFCommonProtoHeader *header;
    SFProtoRebindChannelReq *req;
    SFResponseInfo response;
    int result;

    header = (SFCommonProtoHeader *)out_buff;
    req = (SFProtoRebindChannelReq *)(header + 1);
    int2buff(channel_id, req->channel_id);
    int2buff(key, req->key);
    SF_PROTO_SET_HEADER(header, SF_SERVICE_PROTO_REBIND_CHANNEL_REQ,
            sizeof(SFProtoRebindChannelReq));
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff,
                    sizeof(out_buff), &response, network_timeout,
                    SF_SERVICE_PROTO_REBIND_CHANNEL_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}
