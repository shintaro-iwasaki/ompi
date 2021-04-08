/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2016 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2019      Sandia National Laboratories.  All rights reserved.
 * Copyright (c) 2021      Argonne National Laboratory.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "opal_config.h"
#include "opal/mca/threads/argobots/threads_argobots.h"

#include <errno.h>
#include <string.h>

#include "opal/constants.h"
#include "opal/mca/threads/argobots/threads_argobots_mutex.h"
#include "opal/mca/threads/mutex.h"

/*
 * Wait and see if some upper layer wants to use threads, if support
 * exists.
 */
bool opal_uses_threads = false;

static void mca_threads_argobots_mutex_constructor(opal_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = opal_thread_internal_mutex_init(&p_mutex->m_lock_argobots, false);
    assert(0 == ret);
    p_mutex->m_lock_debug = 0;
    p_mutex->m_lock_file = NULL;
    p_mutex->m_lock_line = 0;
#else
    opal_thread_internal_mutex_init(&p_mutex->m_lock_argobots, false);
#endif
    opal_atomic_lock_init(&p_mutex->m_lock_atomic, 0);
}

static void mca_threads_argobots_mutex_destructor(opal_mutex_t *p_mutex)
{
    opal_thread_internal_mutex_destroy(&p_mutex->m_lock_argobots);
}

static void mca_threads_argobots_recursive_mutex_constructor(opal_recursive_mutex_t *p_mutex)
{
#if OPAL_ENABLE_DEBUG
    int ret = opal_thread_internal_mutex_init(&p_mutex->m_lock_argobots, true);
    assert(0 == ret);
    p_mutex->m_lock_debug = 0;
    p_mutex->m_lock_file = NULL;
    p_mutex->m_lock_line = 0;
#else
    opal_thread_internal_mutex_init(&p_mutex->m_lock_argobots, true);
#endif
    opal_atomic_lock_init(&p_mutex->m_lock_atomic, 0);
}

static void mca_threads_argobots_recursive_mutex_destructor(opal_recursive_mutex_t *p_mutex)
{
    opal_thread_internal_mutex_destroy(&p_mutex->m_lock_argobots);
}

OBJ_CLASS_INSTANCE(opal_mutex_t, opal_object_t, mca_threads_argobots_mutex_constructor,
                   mca_threads_argobots_mutex_destructor);
OBJ_CLASS_INSTANCE(opal_recursive_mutex_t, opal_object_t,
                   mca_threads_argobots_recursive_mutex_constructor,
                   mca_threads_argobots_recursive_mutex_destructor);

int opal_cond_init(opal_cond_t *cond)
{
    return opal_thread_internal_cond_init(cond);
}

int opal_cond_wait(opal_cond_t *cond, opal_mutex_t *lock)
{
    opal_thread_internal_cond_wait(cond, &lock->m_lock_argobots);
    return OPAL_SUCCESS;
}

int opal_cond_broadcast(opal_cond_t *cond)
{
    opal_thread_internal_cond_broadcast(cond);
    return OPAL_SUCCESS;
}

int opal_cond_signal(opal_cond_t *cond)
{
    opal_thread_internal_cond_signal(cond);
    return OPAL_SUCCESS;
}

int opal_cond_destroy(opal_cond_t *cond)
{
    opal_thread_internal_cond_destroy(cond);
    return OPAL_SUCCESS;
}
