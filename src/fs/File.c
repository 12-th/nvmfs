#include "File.h"

// struct file_operations
// {
//     struct module * owner;
//     loff_t (*llseek)(struct file *, loff_t, int);
//     ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
//     ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
//     int (*iterate)(struct file *, struct dir_context *);
//     int (*open)(struct inode *, struct file *);
//     int (*release)(struct inode *, struct file *);
//     // long (*fallocate)(struct file * file, int mode, loff_t offset, loff_t len);
// };