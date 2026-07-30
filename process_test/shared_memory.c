/*
 * shared_memory.c — POSIX 共享内存完整示例
 *
 * 演示流程：
 *   1. 使用 shm_open()  创建/打开一个共享内存对象
 *   2. 使用 ftruncate() 设置共享内存的大小
 *   3. 使用 mmap()      将共享内存映射到当前进程的地址空间
 *   4. fork() 创建子进程，父子进程通过共享内存通信
 *   5. 使用 munmap() / shm_unlink() 正确清理资源
 *
 * 编译：gcc -o shared_memory shared_memory.c -lrt
 * 运行：./shared_memory
 *
 * 注意：POSIX 共享内存对象在 Linux 上由 /dev/shm/ 下的文件支撑，
 *       编译时需要链接实时库 -lrt（部分旧系统需要，较新的 glibc 已内置）。
 */

#include <stdio.h>      /* printf, perror, fprintf           */
#include <stdlib.h>     /* exit, EXIT_FAILURE, EXIT_SUCCESS  */
#include <unistd.h>     /* fork, ftruncate, close, getpid    */
#include <string.h>     /* strcpy, strlen                    */
#include <sys/mman.h>   /* shm_open, mmap, munmap, shm_unlink,
                           PROT_READ, PROT_WRITE,
                           MAP_SHARED, MAP_FAILED            */
#include <sys/wait.h>   /* waitpid                          */
#include <fcntl.h>      /* O_RDWR, O_CREAT, O_EXCL          */
#include <errno.h>      /* errno                            */

/* ------------------------------------------------------------------ *
 * 共享内存配置
 * ------------------------------------------------------------------ */
#define SHM_NAME   "/demo_shm"     /* 共享内存名称，必须以 '/' 开头    */
#define SHM_SIZE   1024            /* 共享内存大小（字节）             */

/* ------------------------------------------------------------------ *
 * 主函数
 * ------------------------------------------------------------------ */
int main(void)
{
    /*
     * ------------------------------------------------------------------
     * 第一步：创建/打开共享内存对象
     *
     * shm_open() 的行为类似 open()，但操作的是共享内存对象而非普通文件。
     *
     *   - 第一个参数 name：共享内存名称，必须以 '/' 开头，且名称中不能
     *     再包含 '/'。内核实际会在 /dev/shm/ 下创建同名文件。
     *   - 第二个参数 oflag：打开标志。
     *       O_RDWR  : 读写模式打开
     *       O_CREAT : 如果不存在则创建
     *       O_EXCL  : 与 O_CREAT 联用，若对象已存在则返回错误（防止
     *                 重复创建，本示例中演示用，首次运行正常，再次运行
     *                 需要先删除 /dev/shm/demo_shm 或调用 shm_unlink）
     *   - 第三个参数 mode：新建对象时的权限位，类似 chmod。
     *     即使使用了 O_EXCL，mode 参数在创建时仍然生效。
     *
     * 返回值：成功返回文件描述符（非负整数），失败返回 -1 并设置 errno。
     * ------------------------------------------------------------------ */
    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd == -1) {
        /*
         * 如果共享内存对象已存在（上次运行未清理），尝试先删除再重新创建。
         * 这是实用做法，生产代码中应根据需求决定是否覆盖。
         */
        if (errno == EEXIST) {
            fprintf(stderr,
                    "[提示] 共享内存 '%s' 已存在，正在删除后重新创建...\n",
                    SHM_NAME);
            shm_unlink(SHM_NAME);
            fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
        }
        if (fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
    }
    printf("[父进程 %d] 共享内存对象创建成功, fd = %d\n", getpid(), fd);

    /*
     * ------------------------------------------------------------------
     * 第二步：设置共享内存大小
     *
     * 新创建的共享内存对象大小为 0，必须通过 ftruncate() 设置大小后才能
     * 使用 mmap() 映射。大小会被向上取整到系统页大小（通常 4096 字节）。
     *
     * ftruncate() 也可以用来扩大或缩小已有共享内存的大小。
     * ------------------------------------------------------------------ */
    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    printf("[父进程 %d] 共享内存大小设置为 %d 字节\n", getpid(), SHM_SIZE);

    /*
     * ------------------------------------------------------------------
     * 第三步：将共享内存映射到进程地址空间
     *
     * mmap() 将共享内存对象映射到当前进程的虚拟地址空间，之后就能像操作
     * 普通内存一样通过指针来读写共享内存。
     *
     * 参数说明：
     *   addr   : NULL — 让内核选择映射地址
     *   length : 映射长度，不能超过 ftruncate 设置的大小
     *   prot   : PROT_READ | PROT_WRITE — 可读可写
     *   flags  : MAP_SHARED — 对映射区的修改会写回底层共享对象，
     *            其他进程的映射也能看到这些修改（这是 IPC 的关键）
     *   fd     : 共享内存的文件描述符
     *   offset : 0 — 从共享内存对象的起始位置开始映射
     *
     * 返回值：成功返回映射区的起始地址，失败返回 MAP_FAILED((void *)-1)
     * ------------------------------------------------------------------ */
    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    printf("[父进程 %d] 共享内存映射成功, 地址 = %p\n", getpid(),
           (void *) shm_ptr);

    /*
     * ------------------------------------------------------------------
     * 第四步：close(fd)
     *
     * mmap() 建立映射后，就可以关闭文件描述符了——内核会维护映射的引用
     * 计数，close(fd) 不会影响已建立的映射。提前关闭可以避免忘记释放
     * 文件描述符资源。
     * ------------------------------------------------------------------ */
    close(fd);
    printf("[父进程 %d] 文件描述符已关闭（映射仍有效）\n", getpid());

    /*
     * ------------------------------------------------------------------
     * 第五步：fork() 创建子进程，演示父子进程通过共享内存通信
     *
     * fork() 后子进程会继承父进程的 mmap 映射，因此父子进程可以通过
     * 同一个共享内存区域交换数据。这里演示的场景是：
     *   - 父进程向共享内存写入一条消息
     *   - 子进程从共享内存中读取这条消息并打印
     *   - 父进程通过 waitpid() 等待子进程处理完毕
     * ------------------------------------------------------------------ */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* ========================================================== *
         * 子进程分支
         * ========================================================== */
        printf("[子进程 %d] 启动，等待父进程写入数据...\n", getpid());

        /*
         * 简易同步：轮询等待父进程写入。
         *
         * 生产代码中应使用 POSIX 信号量（sem_open/sem_wait）或条件变量
         * 来实现可靠的进程间同步，这里的轮询仅用于教学演示。
         * 父进程通过将共享内存第一个字节设为非 '\0' 来通知子进程。
         */
        while (shm_ptr[0] == '\0') {
            /* 忙等待——仅用于演示，生产环境请勿这样做 */
        }

        /* 读取并打印共享内存中的消息 */
        printf("[子进程 %d] 收到消息: %s\n", getpid(), shm_ptr);

        /*
         * 子进程清理自己的映射。
         *
         * 注意：子进程调用 munmap() 只会解除自己的映射，不会影响父进程
         * 的映射，也不会删除底层的共享内存对象。
         */
        if (munmap(shm_ptr, SHM_SIZE) == -1) {
            perror("子进程 munmap");
        }
        printf("[子进程 %d] 退出\n", getpid());
        exit(EXIT_SUCCESS);

    } else {
        /* ========================================================== *
         * 父进程分支
         * ========================================================== */

        /* 构造要发送的消息 */
        const char *message = "Hello from parent process via shared memory!";
        printf("[父进程 %d] 写入消息: \"%s\"\n", getpid(), message);

        /* 将消息复制到共享内存（包含结尾的 '\0'） */
        strcpy(shm_ptr, message);

        /*
         * 等待子进程处理完毕。
         *
         * waitpid() 阻塞直到子进程退出，确保父进程不会在子进程读取
         * 完数据之前就清理共享内存。
         */
        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
        }
        printf("[父进程 %d] 子进程已结束\n", getpid());

        /*
         * ------------------------------------------------------------------
         * 第六步：清理资源
         *
         * 清理顺序很重要：
         *   1. munmap()  — 解除当前进程的映射
         *   2. shm_unlink() — 删除共享内存对象（名称与 shm_open 一致）
         *
         * shm_unlink() 会从 /dev/shm/ 中移除该对象，但对象只有在其所有
         * 引用（所有进程的映射 + 所有打开的文件描述符）都释放后才真正销毁。
         * 这样即使父进程先 munmap 再 unlink，已经映射了该对象的子进程仍然
         * 可以正常访问。
         * ------------------------------------------------------------------ */
        if (munmap(shm_ptr, SHM_SIZE) == -1) {
            perror("父进程 munmap");
        }
        printf("[父进程 %d] 映射已解除 (munmap)\n", getpid());

        if (shm_unlink(SHM_NAME) == -1) {
            perror("shm_unlink");
        } else {
            printf("[父进程 %d] 共享内存对象 '%s' 已删除 (shm_unlink)\n",
                   getpid(), SHM_NAME);
        }
    }

    return 0;
}
