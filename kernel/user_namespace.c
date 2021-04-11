/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2 of the
 *  License.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/nsproxy.h>
#include <linux/user_namespace.h>

struct user_namespace init_user_ns = {
    .kref = {
        .refcount   = ATOMIC_INIT(2),
    },
    .root_user = &root_user,
};

EXPORT_SYMBOL_GPL(init_user_ns);

#ifdef CONFIG_USER_NS

/*
 * Clone a new ns copying an original user ns, setting refcount to 1
 * @old_ns: namespace to clone
 * Return NULL on error (failure to kmalloc), new ns otherwise
 */
// 克隆 1 个用户命名空间时，为当前用户和 root 都创建了新的 user_sturct 实例；
static struct user_namespace *clone_user_ns(struct user_namespace *old_ns)
{
    struct user_namespace *ns;
    struct user_struct *new_user;
    int n;

    ns = kmalloc(sizeof(struct user_namespace), GFP_KERNEL);
    if (!ns)
        return ERR_PTR(-ENOMEM);

    kref_init(&ns->kref);

    for (n = 0; n < UIDHASH_SZ; ++n)
        INIT_HLIST_HEAD(ns->uidhash_table + n);

    /* Insert new root user.  */
    ns->root_user = alloc_uid(ns, 0);
    if (!ns->root_user) {
        kfree(ns);
        return ERR_PTR(-ENOMEM);
    }

    /* Reset current->user with a new one */
    // 将 current->user 替换为新的；
    // alloc_uid 是 1 个辅助函数，对当前命名空间中给定 UID 的用户，如果用户没有对应的 user_struct 实例，则分配一个新的实例；
    new_user = alloc_uid(ns, current->uid);

    if (!new_user) {
        free_uid(ns->root_user);
        kfree(ns);
        return ERR_PTR(-ENOMEM);
    }

    // 在为 root 和用户分别设置了 user_struct 实例后，switch_uid 确保从现在开始，新的 user_struct 实例用于资源统计；
    // 实质上，就是将 struct task_struct 的 user 成员指向新的 user_struct 实例；
    switch_uid(new_user);

    // 如果内核编译时没有指定支持的用户命名空间，那么复制用户命名空间实际上是空操作；
    // 即总会使用默认的命名空间；

    return ns;
}

struct user_namespace * copy_user_ns(int flags, struct user_namespace *old_ns)
{
    struct user_namespace *new_ns;

    BUG_ON(!old_ns);
    get_user_ns(old_ns);

    if (!(flags & CLONE_NEWUSER))
        return old_ns;

    new_ns = clone_user_ns(old_ns);

    put_user_ns(old_ns);
    return new_ns;
}

void free_user_ns(struct kref *kref)
{
    struct user_namespace *ns;

    ns = container_of(kref, struct user_namespace, kref);
    release_uids(ns);
    kfree(ns);
}

#endif /* CONFIG_USER_NS */
