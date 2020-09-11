//sf_define.h

#ifndef _SF_DEFINE_H_
#define _SF_DEFINE_H_

#include "fastcommon/sockopt.h"

#define SF_DEF_THREAD_STACK_SIZE (64 * 1024)
#define SF_DEF_MAX_PACKAGE_SIZE  (16 * 1024)
#define SF_DEF_MIN_BUFF_SIZE  (64 * 1024)
#define SF_DEF_MAX_BUFF_SIZE  (64 * 1024)

#define SF_NIO_STAGE_INIT        0  //set ioevent
#define SF_NIO_STAGE_CONNECT     1  //do connect  (client only)
#define SF_NIO_STAGE_HANDSHAKE   2  //notify the thread to handshake (client only)
#define SF_NIO_STAGE_RECV        4  //do recv
#define SF_NIO_STAGE_SEND        8  //do send
#define SF_NIO_STAGE_FORWARDED  16  //deal the forwarded request
#define SF_NIO_STAGE_CONTINUE   32  //notify the thread continue deal
#define SF_NIO_STAGE_CLOSE     256  //cleanup the task

#define SF_NIO_FLAG_INPROGRESS        1024
#define SF_NIO_STAGE_FLAGS            (SF_NIO_FLAG_INPROGRESS)
#define SF_NIO_STAGE_RECV_INPROGRESS  (SF_NIO_STAGE_RECV | SF_NIO_FLAG_INPROGRESS)
#define SF_NIO_STAGE_SEND_INPROGRESS  (SF_NIO_STAGE_SEND | SF_NIO_FLAG_INPROGRESS)

#define SF_NIO_TASK_STAGE_FETCH(task)  __sync_add_and_fetch(&task->nio_stage, 0)
#define SF_NIO_STAGE_ONLY(stage)       (stage & (~SF_NIO_STAGE_FLAGS))

#define SF_NIO_STAGE_IS_INPROGRESS(stage)  \
    ((stage & SF_NIO_FLAG_INPROGRESS) != 0)

#define SF_RETRIABLE_ERROR_MIN             9901
#define SF_RETRIABLE_ERROR_MAX             9988
#define SF_RETRIABLE_ERROR_NO_SERVER       9901  //no server available
#define SF_RETRIABLE_ERROR_NOT_MASTER      9902  //i am not master
#define SF_RETRIABLE_ERROR_NOT_ACTIVE      9903  //i am not active
#define SF_RETRIABLE_ERROR_CHANNEL_INVALID 9904  //client should re-setup channel

#define SF_IS_RETRIABLE_ERROR(code) \
    ((code >= SF_RETRIABLE_ERROR_MIN && code <= SF_RETRIABLE_ERROR_MAX) || \
     (code == EAGAIN) || is_network_error(code))

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif

