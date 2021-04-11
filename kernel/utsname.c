/*
 *  Copyright (C) 2004 IBM Corporation
 *
 *  Author: Serge Hallyn <serue@us.ibm.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2 of the
 *  License.
 */

#include <linux/module.h>
#include <linux/uts.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/err.h>

/*
 * Clone a new ns copying an original utsname, setting refcount to 1
 * @old_ns: namespace to clone
 * Return NULL on error (failure to kmalloc), new ns otherwise
 */
static struct uts_namespace *clone_uts_ns(struct uts_namespace *old_ns)
{
    struct uts_namespace *ns;

    ns = kmalloc(sizeof(struct uts_namespace), GFP_KERNEL);
    if (!ns)
        return ERR_PTR(-ENOMEM);

    down_read(&uts_sem);
    memcpy(&ns->name, &old_ns->name, sizeof(ns->name));
    up_read(&uts_sem);
    kref_init(&ns->kref);
    return ns;
}

/*
 * Copy task tsk's utsname namespace, or clone it if flags
 * specifies CLONE_NEWUTS.  In latter case, changes to the
 * utsname of this process won't be seen by parent, and vice
 * versa.
 */
// 创建一个新的 uts 命名空间；
// 在某个进程调用 fork 并通过 CLONE_NEWUTS 标志指定创建新的 uts 命名空间时，调用该函数；
//  在该情况下，会生成先前的 uts_namespace 实例的一份副本；
//  当前进程的 nsproxy 实例内部的指针会指向新副本；
struct uts_namespace *copy_utsname(unsigned long flags, struct uts_namespace *old_ns)
{
    struct uts_namespace *new_ns;

    BUG_ON(!old_ns);
    get_uts_ns(old_ns);

    if (!(flags & CLONE_NEWUTS))
        return old_ns;

    new_ns = clone_uts_ns(old_ns);

    put_uts_ns(old_ns);
    return new_ns;
}

void free_uts_ns(struct kref *kref)
{
    struct uts_namespace *ns;

    ns = container_of(kref, struct uts_namespace, kref);
    kfree(ns);
}
