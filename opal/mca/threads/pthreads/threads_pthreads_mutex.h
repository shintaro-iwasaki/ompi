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
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef OPAL_MCA_THREADS_PTHREADS_THREADS_PTHREADS_MUTEX_H
#define OPAL_MCA_THREADS_PTHREADS_THREADS_PTHREADS_MUTEX_H

/**
 * @file:
 *
 * Mutual exclusion functions: Unix implementation.
 *
 * Functions for locking of critical sections.
 *
 * On unix, use pthreads or our own atomic operations as
 * available.
 */

#include "opal_config.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include "opal/class/opal_object.h"
#include "opal/sys/atomic.h"
#include "opal/util/output.h"

BEGIN_C_DECLS

typedef pthread_mutex_t opal_thread_internal_mutex_t;

#define OPAL_THREAD_INTERNAL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#if defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
#    define OPAL_THREAD_INTERNAL_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#elif defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
#    define OPAL_THREAD_INTERNAL_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

static inline int opal_thread_internal_mutex_init(opal_thread_internal_mutex_t *p_mutex,
                                                  bool recursive)
{
    int ret;
#if OPAL_ENABLE_DEBUG
    if (recursive) {
        pthread_mutexattr_t mutex_attr;
        ret = pthread_mutexattr_init(&mutex_attr);
        if (0 != ret)
            return OPAL_ERR_IN_ERRNO;
        ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        if (0 != ret) {
            ret = pthread_mutexattr_destroy(&mutex_attr);
            assert(0 == ret);
            return OPAL_ERR_IN_ERRNO;
        }
        ret = pthread_mutex_init(p_mutex, &mutex_attr);
        if (0 != ret) {
            ret = pthread_mutexattr_destroy(&mutex_attr);
            assert(0 == ret);
            return OPAL_ERR_IN_ERRNO;
        }
        ret = pthread_mutexattr_destroy(&mutex_attr);
        assert(0 == ret);
    } else {
        ret = pthread_mutex_init(p_mutex, NULL);
    }
#else
    if (recursive) {
        pthread_mutexattr_t mutex_attr;
        ret = pthread_mutexattr_init(&mutex_attr);
        if (0 != ret) {
            return OPAL_ERR_IN_ERRNO;
        }
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init(p_mutex, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
    } else {
        ret = pthread_mutex_init(p_mutex, NULL);
    }
#endif
    return 0 == ret ? OPAL_SUCCESS : OPAL_ERR_IN_ERRNO;
}

static inline void opal_thread_internal_mutex_lock(opal_thread_internal_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_mutex_lock(p_mutex);
    if (EDEADLK == ret) {
        opal_output(0, "opal_thread_internal_mutex_lock() %d", ret);
    }
    assert(0 == ret);
#else
    pthread_mutex_lock(p_mutex);
#endif
}

static inline int opal_thread_internal_mutex_trylock(opal_thread_internal_mutex_t *p_mutex)
{
    int ret = pthread_mutex_trylock(p_mutex);
#if OPAL_ENABLE_DEBUG
    if (EDEADLK == ret) {
        opal_output(0, "opal_thread_internal_mutex_trylock() %d", ret);
        return 1;
    }
#endif
    return 0 == ret ? 0 : 1;
    ;
}

static inline void opal_thread_internal_mutex_unlock(opal_thread_internal_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_mutex_unlock(p_mutex);
    if (EDEADLK == ret) {
        opal_output(0, "opal_thread_internal_mutex_unlock() %d", ret);
    }
    assert(0 == ret);
#else
    pthread_mutex_unlock(p_mutex);
#endif
}

static inline void opal_thread_internal_mutex_destroy(opal_thread_internal_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_mutex_destroy(p_mutex);
    assert(0 == ret);
#else
    pthread_mutex_destroy(p_mutex);
#endif
}

typedef pthread_cond_t opal_thread_internal_cond_t;

#define OPAL_THREAD_INTERNAL_COND_INITIALIZER PTHREAD_COND_INITIALIZER

static inline int opal_thread_internal_cond_init(opal_thread_internal_cond_t *p_cond)
{
    int ret = pthread_cond_init(p_cond, NULL);
    return 0 == ret ? OPAL_SUCCESS : OPAL_ERR_IN_ERRNO;
}

static inline void opal_thread_internal_cond_wait(opal_thread_internal_cond_t *p_cond,
                                                  opal_thread_internal_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_cond_wait(p_cond, p_mutex);
    assert(0 == ret);
#else
    pthread_cond_wait(p_cond, p_mutex);
#endif
}

static inline void opal_thread_internal_cond_broadcast(opal_thread_internal_cond_t *p_cond)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_cond_broadcast(p_cond);
    assert(0 == ret);
#else
    pthread_cond_broadcast(p_cond);
#endif
}

static inline void opal_thread_internal_cond_signal(opal_thread_internal_cond_t *p_cond)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_cond_signal(p_cond);
    assert(0 == ret);
#else
    pthread_cond_signal(p_cond);
#endif
}

static inline void opal_thread_internal_cond_destroy(opal_thread_internal_cond_t *p_cond)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_cond_destroy(p_cond);
    assert(0 == ret);
#else
    pthread_cond_destroy(p_cond);
#endif
}

struct opal_mutex_t {
    opal_object_t super;

    pthread_mutex_t m_lock_pthread;

#if OPAL_ENABLE_DEBUG
    int m_lock_debug;
    const char *m_lock_file;
    int m_lock_line;
#endif

    opal_atomic_lock_t m_lock_atomic;
};
OPAL_DECLSPEC OBJ_CLASS_DECLARATION(opal_mutex_t);
OPAL_DECLSPEC OBJ_CLASS_DECLARATION(opal_recursive_mutex_t);

#if defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
#    define OPAL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#elif defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
#    define OPAL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

#if OPAL_ENABLE_DEBUG
#    define OPAL_MUTEX_STATIC_INIT                                                               \
        {                                                                                        \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                         \
            .m_lock_pthread = PTHREAD_MUTEX_INITIALIZER, .m_lock_debug = 0, .m_lock_file = NULL, \
            .m_lock_line = 0, .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                            \
        }
#else
#    define OPAL_MUTEX_STATIC_INIT                                                               \
        {                                                                                        \
            .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                         \
            .m_lock_pthread = PTHREAD_MUTEX_INITIALIZER, .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT, \
        }
#endif

#if defined(OPAL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER)

#    if OPAL_ENABLE_DEBUG
#        define OPAL_RECURSIVE_MUTEX_STATIC_INIT                                               \
            {                                                                                  \
                .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                                   \
                .m_lock_pthread = OPAL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER, .m_lock_debug = 0, \
                .m_lock_file = NULL, .m_lock_line = 0, .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT, \
            }
#    else
#        define OPAL_RECURSIVE_MUTEX_STATIC_INIT                            \
            {                                                               \
                .super = OPAL_OBJ_STATIC_INIT(opal_mutex_t),                \
                .m_lock_pthread = OPAL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER, \
                .m_lock_atomic = OPAL_ATOMIC_LOCK_INIT,                     \
            }
#    endif

#endif

/************************************************************************
 *
 * mutex operations (non-atomic versions)
 *
 ************************************************************************/

static inline int opal_mutex_trylock(opal_mutex_t *m)
{
    int ret = pthread_mutex_trylock(&m->m_lock_pthread);
    if (EDEADLK == ret) {
#if OPAL_ENABLE_DEBUG
        opal_output(0, "opal_mutex_trylock() %d", ret);
#endif
        return 1;
    }
    return 0 == ret ? 0 : 1;
}

static inline void opal_mutex_lock(opal_mutex_t *m)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_mutex_lock(&m->m_lock_pthread);
    if (EDEADLK == ret) {
        errno = ret;
        opal_output(0, "opal_mutex_lock() %d", ret);
    }
#else
    pthread_mutex_lock(&m->m_lock_pthread);
#endif
}

static inline void opal_mutex_unlock(opal_mutex_t *m)
{
#if OPAL_ENABLE_DEBUG
    int ret = pthread_mutex_unlock(&m->m_lock_pthread);
    if (EPERM == ret) {
        errno = ret;
        opal_output(0, "opal_mutex_unlock() %d", ret);
    }
#else
    pthread_mutex_unlock(&m->m_lock_pthread);
#endif
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

typedef pthread_cond_t opal_cond_t;
#define OPAL_CONDITION_STATIC_INIT PTHREAD_COND_INITIALIZER

int opal_cond_init(opal_cond_t *cond);
int opal_cond_wait(opal_cond_t *cond, opal_mutex_t *lock);
int opal_cond_broadcast(opal_cond_t *cond);
int opal_cond_signal(opal_cond_t *cond);
int opal_cond_destroy(opal_cond_t *cond);

END_C_DECLS

#endif /* OPAL_MCA_THREADS_PTHREADS_THREADS_PTHREADS_MUTEX_H */
