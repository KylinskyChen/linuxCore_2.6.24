#ifndef _LINUX_UTSNAME_H
#define _LINUX_UTSNAME_H

#define __OLD_UTS_LEN 8

struct oldold_utsname {
    char sysname[9];
    char nodename[9];
    char release[9];
    char version[9];
    char machine[9];
};

#define __NEW_UTS_LEN 64

struct old_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

// uts_namespace 提供的属性信息；
// 使用 uname 工具可以取得这些属性值；
// 也可以在 /proc/sys/kernel 中看到：
//      cat /proc/sys/kernel/ostype
//      cat /proc/sys/kernel/osrelease
struct new_utsname {
    char sysname[65];       // 系统的名称 (Linux...)；
    char nodename[65];
    char release[65];       //内核版本发布；
    char version[65];
    char machine[65];       // 机器名；
    char domainname[65];
};

#ifdef __KERNEL__

#include <linux/sched.h>
#include <linux/kref.h>
#include <linux/nsproxy.h>
#include <asm/atomic.h>

// uts 命名空间几乎不需要特别的处理，只需要简单量，无层次组织，所有的信息都汇集在结构体中；
struct uts_namespace {
    struct kref kref;           // 1 个嵌入的引用计数器，可用于跟踪内核有多少地方使用了 struct uts_namespace 的实例；
    struct new_utsname name;    // uts_namespace 所提供的属性信息本身包含在这里；
};
extern struct uts_namespace init_uts_ns;

static inline void get_uts_ns(struct uts_namespace *ns)
{
    kref_get(&ns->kref);
}

extern struct uts_namespace *copy_utsname(unsigned long flags,
                    struct uts_namespace *ns);
extern void free_uts_ns(struct kref *kref);

static inline void put_uts_ns(struct uts_namespace *ns)
{
    kref_put(&ns->kref, free_uts_ns);
}
static inline struct new_utsname *utsname(void)
{
    return &current->nsproxy->uts_ns->name;
}

static inline struct new_utsname *init_utsname(void)
{
    return &init_uts_ns.name;
}

extern struct rw_semaphore uts_sem;

#endif /* __KERNEL__ */

#endif /* _LINUX_UTSNAME_H */
