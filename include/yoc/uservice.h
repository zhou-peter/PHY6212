/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef YOC_USERVICE_H
#define YOC_USERVICE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <yoc/event.h>
#include <aos/list.h>
#include <aos/kernel.h>

#define QUEUE_MSG_COUNT 8

typedef struct uservice   uservice_t;
typedef struct utask      utask_t;
typedef struct rpc_buffer rpc_buffer_t;

/**
 * rpc_t - remote procedure call struct
 *
 * @srv:
 * @cmd_id:
 * @timeout_ms:
 * @data:
 */
typedef struct _rpc_t {
    uservice_t   *srv;
    uint32_t      cmd_id;
    rpc_buffer_t *data;
} rpc_t;

typedef int (*process_t)(void *context, rpc_t *rpc);

typedef struct {
    int cmd_id;
   process_t process;
} rpc_process_t;

struct utask {
    aos_task_t  task;
    int8_t      running;

    uint8_t     queue_count;
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_UTASK)
    uint8_t     queue_max_used;
    slist_t     rpc_reclist;
#endif
    void       *queue_buffer;
    aos_queue_t queue;
    aos_mutex_t mutex;
    rpc_t      *current_rpc;
    aos_sem_t   running_wait;

    slist_t     uservice_lists;
    slist_t     rpc_buffer_gc_cache;
    slist_t     node;
};

/**
 * rpc_init() - initialise a rpc
 *
 * This function initialise a rpc. Returns zero or a negative error code
 *
 * @rpc: the rpc to initialise, Cannot be an empty pointer.
 * @cmd_id: the rpc command id
 * @timeout_ms: the rpc call wait time.
 *   >0: wait time (ms)
 *   =0: is asynchronous rpc comamnd
 *   =AOS_WAIT_FOREVER: is synchronous rpc command
 *
 * Returns:
 *   %0 on success
 *   -%ENOMEM when memory allocation fails.
 *   -%EINVAL on an invalid param.
 *
 */
int  rpc_init(rpc_t *rpc, int cmd_id, int timeout_ms);

/**
 * rpc_put_reset - reset rpc data buffer
 *
 * This function destroy the rpc data buffer, and reset put status
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 *
 */
void rpc_put_reset(rpc_t *rpc);

/**
 * rpc_put_int - put integer value into the rpc
 * rpc_put_uint8 - put uint8_t value into the rpc
 * rpc_put_double - put double value into the rpc
 * rpc_put_point - put point value into the rpc
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 * @v: the value
 *
 * Returns:
 *   -%0 on success
 *   -%ENOMEM when memory allocation fails.
 */
int rpc_put_int(rpc_t *rpc, int v);
int rpc_put_uint8(rpc_t *rpc, uint8_t v);
int rpc_put_double(rpc_t *rpc, double v);
int rpc_put_point(rpc_t *rpc, const void *v);

/**
 * rpc_put_buffer - put buffer into the rpc
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 * @data: buffer pointer
 * @size: buffer memory size
 *
 * Returns:
 *   -%0 on success
 *   -%ENOMEM when memory allocation fails.
 *   -%EINVAL on an invalid data or size, data is NULL or size is zero
 */
int rpc_put_buffer(rpc_t *rpc, const void *data, size_t size);

/**
 * rpc_put_buffer - put string into the rpc
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 * @str: string pointer
 *
 * Returns:
 *   -%0 on success
 *   -%ENOMEM when memory allocation fails.
 *   -%EINVAL on an invalid str pointer
 */
int rpc_put_string(rpc_t *rpc, char *str);

/**
 * rpc_get_reset - reset rpc data buffer
 *
 * This function destroy the rpc data buffer, and reset put status
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 *
 */
void rpc_get_reset(rpc_t *rpc);

/**
 * rpc_get_int - put integer value into the rpc
 * rpc_get_uint8 - put uint8_t value into the rpc
 * rpc_get_double - put double value into the rpc
 * rpc_get_string - put point value into the rpc
 * rpc_get_point
 *
 * @rpc: the initialized rpc pointer, cannot be an empty pointer
 * @v: the value
 *
 * Returns:
 *   -%0 on success
 *   -%ENOMEM when memory allocation fails.
 */
int     rpc_get_int(rpc_t *rpc);
uint8_t rpc_get_uint8(rpc_t *rpc);
double  rpc_get_double(rpc_t *rpc);
char   *rpc_get_string(rpc_t *rpc);
void   *rpc_get_point(rpc_t *rpc);


void   *rpc_get_buffer(rpc_t *rpc, int *count);

void rpc_reply(rpc_t *rpc);
void rpc_deinit(rpc_t *rpc);

uservice_t *uservice_new(const char *name, process_t process_rpc, void *context);
void        uservice_destroy(uservice_t *srv);

int      uservice_call_sync(uservice_t *srv, int cmd, void *param, void *resp, size_t resp_size);
int      uservice_call_async(uservice_t *srv, int cmd, void *param, size_t param_size);
int      uservice_call(uservice_t *srv, rpc_t *rpc);
void     uservice_lock(uservice_t *srv);
void     uservice_unlock(uservice_t *srv);
int      uservice_process(void *context, rpc_t *rpc, const rpc_process_t rpcs[]);
void     uservice_subscribe(uservice_t *srv, uint32_t event_id);

utask_t  *utask_new(const char *name, size_t stack_size, int queue_length, int prio);

#define UTASK_QUEUE_BUF_DEFINE(name,queue_count) \
    static uint8_t name[queue_count * sizeof(rpc_t)]

utask_t  *utask_new_ext(utask_t  * utask, const char *name, void *stack_buf, size_t stack_size, void *queue_buf, int queue_length, int prio);

void     utask_destroy(utask_t *task);
void     utask_join(utask_t *task);
void     utask_add(utask_t *task, uservice_t *srv);
void     utask_remove(utask_t *task, uservice_t *srv);
void     utask_lock(utask_t *task);
void     utask_unlock(utask_t *task);
void     utask_set_softwdt_timeout(int ms);

int  event_service_init(utask_t *task);

void event_subscribe(uint32_t event_id, event_callback_t cb, void *context);
void event_unsubscribe(uint32_t event_id, event_callback_t cb, void *context);
void event_publish(uint32_t event_id, void *data);
void event_publish_delay(uint32_t event_id, void *data, int timeout_ms);

void event_subscribe_fd(uint32_t fd, event_callback_t cb, void *context);
void event_unsubscribe_fd(uint32_t fd, event_callback_t cb, void *context);
void event_publish_fd(uint32_t fd, void *data, int sync);

void tasks_debug();

#endif
