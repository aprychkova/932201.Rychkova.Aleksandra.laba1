﻿#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>

#define PROCFS_NAME "tsu"
static struct proc_dir_entry* our_proc_file = NULL;

//функция вычисления количества секунд до китайского нового года
static long calculate_seconds_to_chinese_new_year(void) {
    struct timespec64 ts;
    struct tm new_year;
    ktime_get_real_ts64(&ts);

    //дата ближайшего китайского нового года
    time64_to_tm(ts.tv_sec, 0, &new_year);
    new_year.tm_year = 2025 - 1900; 
    new_year.tm_mon = 1;           
    new_year.tm_mday = 10; 
    new_year.tm_hour = 0;
    new_year.tm_min = 0;
    new_year.tm_sec = 0;

    time64_t chinese_new_year = mktime64(&new_year);
    return chinese_new_year - ts.tv_sec; //разница между текучим временем и времнем наступления китайского нг в секундах
}

// Чтение
static ssize_t procfile_read(
    struct file* file_pointer, char __user* buffer,
    size_t buffer_length, loff_t* offset)
{
    //свет доходит от солнца до земли за 8 минут и 20 секунд = 500 секунд, туда и обратно 1000 секунд
    long seconds_to_cny = calculate_seconds_to_chinese_new_year();
    long cycles = seconds_to_cny / 1000; 
    char output[64];
    int len;

    len = snprintf(output, sizeof(output), "Cycles until Chinese New Year: %ld\n", cycles);

    if (*offset >= len) {
        return 0;
    }

    if (copy_to_user(buffer, output, len)) {
        return -EFAULT;
    }

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
    pr_info("Welcome!\n");

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
    pr_info("Goodbye!\n");
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL"); //лицензия модуля