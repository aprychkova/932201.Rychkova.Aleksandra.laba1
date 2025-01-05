#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define PROCFS_NAME "tsu"
static struct proc_dir_entry* our_proc_file = NULL;

//чтение
static ssize_t procfile_read(
    struct file* file_pointer, char __user* buffer,
    size_t buffer_length, loff_t* offset)
{
    char s[6] = "Tomsk\n"; 
    size_t len = 5 
    //если файл уже прочитан
    if (*offset >= len) {
        return 0;
    }

    (copy_to_user(buffer, s, len)
    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    //обновление смещения
    *offset += len;
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

//загрузка
static int __init procfs1_init(void) {
    pr_info("Welcome to the Tomsk State University\n");

    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (!our_proc_file) {
        pr_err("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    return 0;
}

//выгрузка
static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("Tomsk State University forever!\n");
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL"); //лицензия модуля