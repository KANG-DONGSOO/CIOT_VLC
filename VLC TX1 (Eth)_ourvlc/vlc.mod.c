#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x465ae224, "module_layout" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0xff178f6, "__aeabi_idivmod" },
	{ 0x15692c87, "param_ops_int" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x35525017, "rtdm_timer_start" },
	{ 0x81fba240, "malloc_sizes" },
	{ 0xe2aa256a, "_raw_spin_lock_bh" },
	{ 0xaa61102d, "remove_proc_entry" },
	{ 0xc5f1bca6, "mutex_unlock" },
	{ 0x28118cb6, "__get_user_1" },
	{ 0x91715312, "sprintf" },
	{ 0x7d11c268, "jiffies" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x1d44900a, "proc_mkdir" },
	{ 0xcb4a1f12, "rtdm_timer_destroy" },
	{ 0x27e1a049, "printk" },
	{ 0x31612cd8, "free_netdev" },
	{ 0x328a05f1, "strncpy" },
	{ 0x82f70bdc, "register_netdev" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0xb461a3d5, "skb_push" },
	{ 0x43d91368, "mutex_lock" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0x7f63b31e, "_memcpy_toio" },
	{ 0x8736fc3, "_raw_spin_unlock_bh" },
	{ 0xc2165d85, "__arm_iounmap" },
	{ 0xbb1948a3, "alloc_netdev_mqs" },
	{ 0xa2577d47, "rtdm_tbase" },
	{ 0xa6c00c6e, "create_proc_entry" },
	{ 0xcf14fb1f, "kmem_cache_alloc_trace" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0xa6607abe, "unregister_netdev" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xc826b355, "__netif_schedule" },
	{ 0x99bb8806, "memmove" },
	{ 0x7d8c8d21, "consume_skb" },
	{ 0xac8f37b2, "outer_cache" },
	{ 0xa21a3d82, "__xntimer_init" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "7C0CEAD1EEF7D3FA38543F3");
