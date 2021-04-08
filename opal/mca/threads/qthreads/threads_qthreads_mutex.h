/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2018 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2015-2016 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2019      Sandia National Laboratories.  All rights reserved.
 * Copyright (c) 2020      Triad National Security, LLC. All rights
 *                         reserved.
 *
 * Copyright (c) 2020      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef OPAL_MCA_THREADS_QTHREADS_THREADS_QTHREADS_MUTEX_H
#define OPAL_MCA_THREADS_QTHREADS_THREADS_QTHREADS_MUTEX_H 1

#include "opal_config.h"

#include <errno.h>
#include <stdio.h>

#include "opal/class/opal_object.h"
#include "opal/constants.h"
#include "opal/mca/threads/qthreads/threads_qthreads.h"
#include "opal/sys/atomic.h"
#include "opal/util/output.h"

BEGIN_C_DECLS

typedef opal_atomic_lock_t opal_thread_internal_mutex_t;

#define OPAL_THREAD_INTERNAL_MUTEX_INITIALIZER OPAL_ATOMIC_LOCK_INIT
#define OPAL_THREAD_INTERNAL_RECURSIVE_MUTEX_INITIALIZER OPAL_ATOMIC_LOCK_INIT

static inline int opal_thread_internal_mutex_init(opal_thread_internal_mutex_t *p_mutex,
                                                  bool recursive)
{
    opal_atomic_lock_init(p_mutex, 0);
    return OPAL_SUCCESS;
}

static inline void opal_thread_internal_mutex_lock(opal_thread_internal_mutex_t *p_mutex)
{
    opal_threads_ensure_init_qthreads();

    int ret = opal_atomic_trylock(p_mutex);
    while (0 != ret) {
        qthread_yield();
        ret = opal_atomic_trylock(p_mutex);
    }
}

static inline int opal_thread_internal_mutex_trylock(opal_thread_internal_mutex_t *p_mutex)
{
    opal_threads_ensure_init_qthreads();

    int ret = opal_atomic_trylock(p_mutex);
    if (0 != ret) {
        /* Yield to avoid a deadlock. */
        qthread_yield();
    }
    return ret;
}

static inline void opal_thread_internal_mutex_unlock(opal_thread_internal_mutex_t *p_mutex)
{
    opal_threads_ensure_init_qthreads();

    opal_atomic_unlock(p_mutex);
    /* For fairness of locking. */
    qthread_yield();
}

static inline void opal_thread_internal_mutex_destroy(opal_thread_internal_mutex_t *p_mutex)
{
    /* No destructor is needed. */
}

typedef struct opal_thread_cond_waiter_t {
    int m_signaled;
    opal_thread_cond_waiter_t *m_prev;
} opal_thread_cond_waiter_t;

typedef struct {
    opal_atomic_lock_t m_lock;
    opal_thread_cond_waiter_t *m_waiter_head;
    opal_thread_cond_waiter_t *m_waiter_tail;
} opal_thread_internal_cond_t;

#define OPAL_THREAD_INTERNAL_COND_INITIALIZER                                          \
    {                                                                                  \
        .m_lock = OPAL_ATOMIC_LOCK_INIT, .m_waiter_head = NULL, .m_waiter_tail = NULL, \
    }

static inline int opal_thread_internal_cond_init(opal_thread_internal_cond_t *p_cond)
{
    opal_atomic_lock_init(&p_cond->m_lock, 0);
    p_cond->m_waiter_head = NULL;
    p_cond->m_waiter_tail = NULL;
    return OPAL_SUCCESS;
}

static inline void opal_thread_internal_cond_wait(opal_thread_internal_cond_t *p_cond,
                                                  opal_thread_internal_mutex_t *p_mutex)
{
    opal_threads_ensure_init_qthreads();
    /* This thread is taking "lock", so only this thread can access this
     * condition variable.  */
    opal_atomic_lock(&p_cond->m_lock);
    opal_thread_cond_waiter_t waiter = {0, NULL};
    if (NULL == p_cond->m_waiter_head) {
        p_cond->m_waiter_tail = &waiter;
    } else {
        p_cond->m_waiter_head->m_prev = &waiter;
    }
    p_cond->m_waiter_head = &waiter;
    opal_atomic_unlock(&p_cond->m_lock);

    while (1) {
        opal_thread_internal_mutex_unlock(p_mutex);
        qthread_yield();
        opal_thread_internal_mutex_lock(p_mutex);
        /* Check if someone woke me up. */
        opal_atomic_lock(&p_cond->m_lock);
        int signaled = waiter.m_signaled;
        opal_atomic_unlock(&p_cond->m_lock);
        if (1 == signaled) {
            break;
        }
        /* Unlock the lock again. */
    }
    return OPAL_SUCCESS;
}

static inline void opal_thread_internal_cond_broadcast(opal_thread_internal_cond_t *p_cond)
{
    opal_atomic_lock(&cond->m_lock);
    while (NULL != cond->m_waiter_tail) {
        opal_thread_cond_waiter_t *p_cur_tail = cond->m_waiter_tail;
        cond->m_waiter_tail = p_cur_tail->m_prev;
        /* Awaken one of threads in a FIFO manner. */
        p_cur_tail->m_signaled = 1;
    }
    /* No waiters. */
    cond->m_waiter_head = NULL;
    opal_atomic_unlock(&cond->m_lock);
    return OPAL_SUCCESS;
}

static inline void opal_thread_internal_cond_signal(opal_thread_internal_cond_t *p_cond)
{
    opal_atomic_lock(&p_cond->m_lock);
    if (NULL != p_cond->m_waiter_tail) {
        opal_thread_cond_waiter_t *p_cur_tail = p_cond->m_waiter_tail;
        p_cond->m_waiter_tail = p_cur_tail->m_prev;
        /* Awaken one of threads. */
        p_cur_tail->m_signaled = 1;
        if (NULL == p_cond->m_waiter_tail) {
            p_cond->m_waiter_head = NULL;
        }
    }
    opal_atomic_unlock(&p_cond->m_lock);
    return OPAL_SUCCESS;
}

static inline void opal_thread_internal_cond_destroy(opal_thread_internal_cond_t *p_cond)
{
    /* No destructor is needed. */
}

struct opal_mutex_t {
    opal_object_t super;

    opal_atomic_lock_t m_lock;

#if OPAL_ENABLE_DEBUG
    int m_lock_debug;
    const char *m_lock_file;
    int m_lock_line;
#endif

    opal_atomic_lock_t m_lock_atomic;
};

OPAL_DECLSPEC OBJ_CLASS_DECLARATION(opal_mutex_t);
OPAL_DECLSPEC OBJ_CLASS_DECLARATION(opal_recursive_mutex_t);

#if OPAL_ENABLE_DEBUG
#    define OPAL_MUTEX_STATIC_INIT                                                        \
        {                                                                                 \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                  \
            .m_lock = OPAL_THREAD_INTERNAL_MUTEX_INITIALIZER,                             \
            .m_lock_debug = 0, .m_lock_file = NULL, .m_lock_line = 0,                     \
            .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                                       \
        }
#else
#    define OPAL_MUTEX_STATIC_INIT                                                        \
        {                                                                                 \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                  \
            .m_lock = OPAL_THREAD_INTERNAL_MUTEX_INITIALIZER,                             \
            .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                                       \
        }
#endif

#if OPAL_ENABLE_DEBUG
#    define OPAL_RECURSIVE_MUTEX_STATIC_INIT                                              \
        {                                                                                 \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                  \
            .m_lock = OPAL_THREAD_INTERNAL_RECURSIVE_MUTEX_INITIALIZER,                   \
            .m_lock_debug = 0, .m_lock_file = NULL, .m_lock_line = 0,                     \
            .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                                       \
        }
#else
#    define OPAL_RECURSIVE_MUTEX_STATIC_INIT                                              \
        {                                                                                 \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                  \
            .m_lock = OPAL_THREAD_INTERNAL_RECURSIVE_MUTEX_INITIALIZER,                   \
            .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                                       \
        }
#endif

/************************************************************************
 *
 * mutex operations (non-atomic versions)
 *
 ************************************************************************/

static inline void opal_mutex_create(struct opal_mutex_t *m)
{
    opal_threads_ensure_init_qthreads();

    opal_atomic_lock_init(&m->m_lock, 0);
    opal_atomic_lock_init(&m->m_lock_atomic, 0);
#if OPAL_ENABLE_DEBUG
    m->m_lock_debug = 0;
    m->m_lock_file = NULL;
    m->m_lock_line = 0;
#endif
}

static inline void opal_mutex_recursive_create(struct opal_mutex_t *m)
{
    opal_threads_ensure_init_qthreads();

    opal_atomic_lock_init(&m->m_lock, 0);
    opal_atomic_lock_init(&m->m_lock_atomic, 0);
#if OPAL_ENABLE_DEBUG
    m->m_lock_debug = 0;
    m->m_lock_file = NULL;
    m->m_lock_line = 0;
#endif
}

static inline int opal_mutex_trylock(opal_mutex_t *m)
{
    return opal_atomic_trylock(&m->m_lock);
}

static inline void opal_mutex_lock(opal_mutex_t *m)
{
    opal_atomic_lock(&m->m_lock);
}

static inline void opal_mutex_unlock(opal_mutex_t *m)
{
    opal_atomic_unlock(&m->m_lock);
}

/************************************************************************
 *
 * mutex operations (atomic versions)
 *
 ************************************************************************/

#if OPAL_HAVE_ATOMIC_SPINLOCKS

/************************************************************************
 * Spin Locks
 ************************************************************************/

static inline int opal_mutex_atomic_trylock(opal_mutex_t *m)
{
    return opal_atomic_trylock(&m->m_lock_atomic);
}

static inline void opal_mutex_atomic_lock(opal_mutex_t *m)
{
    opal_atomic_lock(&m->m_lock_atomic);
}

static inline void opal_mutex_atomic_unlock(opal_mutex_t *m)
{
    opal_atomic_unlock(&m->m_lock_atomic);
}

#else

/************************************************************************
 * Standard locking
 ************************************************************************/

static inline int opal_mutex_atomic_trylock(opal_mutex_t *m)
{
    return opal_mutex_trylock(m);
}

static inline void opal_mutex_atomic_lock(opal_mutex_t *m)
{
    opal_mutex_lock(m);
}

static inline void opal_mutex_atomic_unlock(opal_mutex_t *m)
{
    opal_mutex_unlock(m);
}

#endif

typedef opal_thread_internal_cond_t opal_cond_t;
#define OPAL_CONDITION_STATIC_INIT OPAL_THREAD_INTERNAL_COND_INITIALIZER

int opal_cond_init(opal_cond_t *cond);
int opal_cond_wait(opal_cond_t *cond, opal_mutex_t *lock);
int opal_cond_broadcast(opal_cond_t *cond);
int opal_cond_signal(opal_cond_t *cond);
int opal_cond_destroy(opal_cond_t *cond);

END_C_DECLS

#endif /* OPAL_MCA_THREADS_QTHREADS_THREADS_QTHREADS_MUTEX_H */
