#ifndef HOOK_H
#define HOOK_H

#include <linux/kprobes.h>

#define HOOK(_name, _function, _original)                                      \
  {                                                                            \
    .name = ("__x64_" _name), .function = (_function),                         \
    .original = (_original),                                                   \
  }

struct ftrace_hook {
  const char *name;
  void *function;
  void *original;
  unsigned long address;
  struct ftrace_ops ops;
};

static unsigned long lookup_name(const char *name) {
  unsigned long retval;
  struct kprobe kp = {.symbol_name = name};

  if (register_kprobe(&kp) < 0)
    return 0;
  retval = (unsigned long)kp.addr;
  unregister_kprobe(&kp);
  return retval;
}

static int fh_resolve_hook_address(struct ftrace_hook *hook) {
  hook->address = lookup_name(hook->name);
  if (!hook->address) {
    pr_debug("unresolved symbol: %s\n", hook->name);
    return -ENOENT;
  }

  *((unsigned long *)hook->original) = hook->address;
  return 0;
}

static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
                                    struct ftrace_ops *ops,
                                    struct ftrace_regs *fregs) {
  struct pt_regs *regs = ftrace_get_regs(fregs);
  struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);

  if (!within_module(parent_ip, THIS_MODULE))
    regs->ip = (unsigned long)hook->function;
}

static int fh_install_hook(struct ftrace_hook *hook) {
  int err;
  if ((err = fh_resolve_hook_address(hook)))
    return err;

  hook->ops.func = fh_ftrace_thunk;
  hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION |
                    FTRACE_OPS_FL_IPMODIFY;

  if ((err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0))) {
    pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
    return err;
  }

  if ((err = register_ftrace_function(&hook->ops))) {
    pr_debug("register_ftrace_function() failed: %d\n", err);
    ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
    return err;
  }

  return 0;
}

static void fh_remove_hook(struct ftrace_hook *hook) {
  int err;

  if ((err = unregister_ftrace_function(&hook->ops)))
    pr_debug("unregister_ftrace_function() failed: %d\n", err);

  if ((err = ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0)))
    pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
}

static int fh_install_hooks(struct ftrace_hook *hooks, size_t count) {
  int err;
  size_t i;

  for (i = 0; i < count; i++) {
    err = fh_install_hook(&hooks[i]);
    if (err)
      goto error;
  }

  return 0;

error:
  while (i != 0) {
    fh_remove_hook(&hooks[--i]);
  }

  return err;
}

static void fh_remove_hooks(struct ftrace_hook *hooks, size_t count) {
  size_t i;

  for (i = 0; i < count; i++)
    fh_remove_hook(&hooks[i]);
}

#endif /* HOOK_H */
