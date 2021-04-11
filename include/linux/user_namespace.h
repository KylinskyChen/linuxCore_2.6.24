#ifndef _LINUX_USER_NAMESPACE_H
#define _LINUX_USER_NAMESPACE_H

#include <linux/kref.h>
#include <linux/nsproxy.h>
#include <linux/sched.h>
#include <linux/err.h>

#define UIDHASH_BITS    (CONFIG_BASE_SMALL ? 3 : 8)
#define UIDHASH_SZ  (1 << UIDHASH_BITS)

// 用户命名空间；
// 用户命名空间在数据结构管理方面类似于 uts；
// 在要求创建 1 个新的用户空间时，生成当前用户命名空间的一份副本，并关联到当前进程的 nsproxy 实例；
struct user_namespace {
    struct kref         kref;                       // 引用计数器；
                                                    // 用于跟踪多少地方使用 user_namespace 实例；
    struct hlist_head   uidhash_table[UIDHASH_SZ];  // 各个实例可以通过该散列表进行访问；
    struct user_struct  *root_user;                 // 负责记录资源消耗；
                                                    // user_struct 维护了一些统计数据，如进程和打开文件的数目；
};

extern struct user_namespace init_user_ns;

#ifdef CONFIG_USER_NS

static inline struct user_namespace *get_user_ns(struct user_namespace *ns)
{
    if (ns)
        kref_get(&ns->kref);
    return ns;
}

extern struct user_namespace *copy_user_ns(int flags,
                       struct user_namespace *old_ns);
extern void free_user_ns(struct kref *kref);

static inline void put_user_ns(struct user_namespace *ns)
{
    if (ns)
        kref_put(&ns->kref, free_user_ns);
}

#else

static inline struct user_namespace *get_user_ns(struct user_namespace *ns)
{
    return &init_user_ns;
}

static inline struct user_namespace *copy_user_ns(int flags,
                          struct user_namespace *old_ns)
{
    if (flags & CLONE_NEWUSER)
        return ERR_PTR(-EINVAL);

    return old_ns;
}

static inline void put_user_ns(struct user_namespace *ns)
{
}

#endif

#endif /* _LINUX_USER_H */
