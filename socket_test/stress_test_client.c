#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 9191
#define DEFAULT_CLIENTS 256
#define DEFAULT_REQUESTS 1000

#define PROGRESS_WIDTH 40
#define POLL_TIMEOUT_MS 100

typedef enum {
    CLIENT_CONNECTING,
    CLIENT_WRITING,
    CLIENT_READING,
    CLIENT_DONE,
    CLIENT_FAILED
} ClientState;

typedef struct {
    int fd;
    int id;

    ClientState state;

    char send_buf[256];
    size_t send_len;
    size_t send_offset;

    char recv_buf[256];
    size_t recv_len;

    uint64_t current_request_start_ns;

    uint64_t request_completed;
    uint64_t request_failed;

    int last_errno;
} Client;

typedef struct {
    uint64_t total_requests;
    uint64_t completed;
    uint64_t failed;

    uint64_t bytes_sent;
    uint64_t bytes_received;

    uint64_t latency_sum_ns;

    uint64_t *latencies_ns;
    size_t latency_count;
    size_t latency_capacity;

    struct timespec start_time;
} Stats;

static volatile sig_atomic_t stop_requested = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static uint64_t now_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    return (uint64_t)ts.tv_sec * 1000000000ULL +
           (uint64_t)ts.tv_nsec;
}

static double elapsed_seconds(struct timespec start)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
        return 0.0;
    }

    time_t sec = now.tv_sec - start.tv_sec;
    long nsec = now.tv_nsec - start.tv_nsec;

    return (double)sec + (double)nsec / 1000000000.0;
}

static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
}

static int connect_nonblock(const char *host, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }

    if (set_nonblock(fd) == -1) {
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        errno = EINVAL;
        return -1;
    }

    int ret = connect(
        fd,
        (struct sockaddr *)&addr,
        sizeof(addr)
    );

    if (ret == 0) {
        return fd;
    }

    if (errno == EINPROGRESS) {
        return fd;
    }

    close(fd);
    return -1;
}

static int verify_connection(Client *c)
{
    int error_code = 0;
    socklen_t len = sizeof(error_code);

    if (getsockopt(
            c->fd,
            SOL_SOCKET,
            SO_ERROR,
            &error_code,
            &len
        ) == -1) {
        return -1;
    }

    if (error_code != 0) {
        errno = error_code;
        return -1;
    }

    return 0;
}

static void stats_init(Stats *stats, uint64_t total_requests)
{
    memset(stats, 0, sizeof(*stats));

    stats->total_requests = total_requests;

    /*
     * 最多记录 1,000,000 个延迟样本。
     * 如果总请求超过这个数量，只记录前 1,000,000 个。
     */
    stats->latency_capacity =
        total_requests > 1000000ULL
            ? 1000000ULL
            : (size_t)total_requests;

    stats->latencies_ns = malloc(
        stats->latency_capacity * sizeof(uint64_t)
    );

    if (stats->latency_capacity != 0 &&
        stats->latencies_ns == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (clock_gettime(
            CLOCK_MONOTONIC,
            &stats->start_time
        ) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
}

static void stats_destroy(Stats *stats)
{
    free(stats->latencies_ns);
    stats->latencies_ns = NULL;
}

static void record_latency(
    Stats *stats,
    uint64_t latency_ns
)
{
    stats->latency_sum_ns += latency_ns;

    if (stats->latency_count <
        stats->latency_capacity) {

        stats->latencies_ns[
            stats->latency_count
        ] = latency_ns;

        stats->latency_count++;
    }
}

static int compare_uint64(
    const void *a,
    const void *b
)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;

    if (x < y) {
        return -1;
    }

    if (x > y) {
        return 1;
    }

    return 0;
}

static double percentile_ms(
    const Stats *stats,
    double percentile
)
{
    if (stats->latency_count == 0) {
        return 0.0;
    }

    uint64_t *tmp = malloc(
        stats->latency_count * sizeof(uint64_t)
    );

    if (tmp == NULL) {
        return 0.0;
    }

    memcpy(
        tmp,
        stats->latencies_ns,
        stats->latency_count * sizeof(uint64_t)
    );

    qsort(
        tmp,
        stats->latency_count,
        sizeof(uint64_t),
        compare_uint64
    );

    double pos =
        percentile / 100.0 *
        (double)(stats->latency_count - 1);

    size_t index = (size_t)pos;
    double fraction = pos - (double)index;

    double a = (double)tmp[index];

    double b;

    if (index + 1 < stats->latency_count) {
        b = (double)tmp[index + 1];
    } else {
        b = a;
    }

    free(tmp);

    return (a + (b - a) * fraction) / 1000000.0;
}

static int prepare_request(
    Client *c,
    uint64_t sequence
)
{
    int len = snprintf(
        c->send_buf,
        sizeof(c->send_buf),
        "bench-%d-%llu\n",
        c->id,
        (unsigned long long)sequence
    );

    if (len < 0 ||
        (size_t)len >= sizeof(c->send_buf)) {
        errno = EMSGSIZE;
        return -1;
    }

    c->send_len = (size_t)len;
    c->send_offset = 0;

    c->recv_len = 0;

    c->current_request_start_ns = now_ns();

    c->state = CLIENT_WRITING;

    return 0;
}

static int handle_connect(Client *c)
{
    if (verify_connection(c) == -1) {
        c->last_errno = errno;
        return -1;
    }

    c->state = CLIENT_WRITING;

    return 0;
}

static int handle_send(
    Client *c,
    Stats *stats
)
{
    while (c->send_offset < c->send_len) {

        ssize_t n = send(
            c->fd,
            c->send_buf + c->send_offset,
            c->send_len - c->send_offset,
            MSG_NOSIGNAL
        );

        if (n > 0) {
            c->send_offset += (size_t)n;
            stats->bytes_sent += (uint64_t)n;
            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        if (n == -1 &&
            (errno == EAGAIN ||
             errno == EWOULDBLOCK)) {

            return 0;
        }

        c->last_errno = errno;
        return -1;
    }

    c->state = CLIENT_READING;

    return 0;
}

static int handle_recv(
    Client *c,
    Stats *stats
)
{
    while (1) {

        /*
         * 留一个空间也没关系，这里只是防止越界。
         */
        if (c->recv_len >= sizeof(c->recv_buf)) {
            c->last_errno = EMSGSIZE;
            return -1;
        }

        ssize_t n = recv(
            c->fd,
            c->recv_buf + c->recv_len,
            sizeof(c->recv_buf) - c->recv_len,
            0
        );

        if (n > 0) {

            c->recv_len += (size_t)n;

            stats->bytes_received += (uint64_t)n;

            /*
             * TCP 是字节流，所以这里不能假设
             * 一次 recv() 就等于一次 response。
             *
             * 查找 '\n'。
             */
            size_t message_end = 0;
            int found = 0;

            for (size_t i = 0;
                 i < c->recv_len;
                 i++) {

                if (c->recv_buf[i] == '\n') {
                    message_end = i + 1;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                continue;
            }

            /*
             * 服务器 Echo 应该完全等于 send_buf。
             */
            if (message_end != c->send_len ||
                memcmp(
                    c->recv_buf,
                    c->send_buf,
                    c->send_len
                ) != 0) {

                c->last_errno = EPROTO;
                return -1;
            }

            /*
             * 本次请求完成。
             */
            uint64_t latency =
                now_ns() -
                c->current_request_start_ns;

            record_latency(stats, latency);

            stats->completed++;

            c->request_completed++;

            c->state = CLIENT_DONE;

            /*
             * 如果后面还有下一条数据，
             * 留在 recv_buf 里的部分应该被保留。
             *
             * 当前 Echo Server 一个请求发一个响应，
             * 正常情况下这里通常刚好完整。
             */
            if (message_end < c->recv_len) {

                memmove(
                    c->recv_buf,
                    c->recv_buf + message_end,
                    c->recv_len - message_end
                );

                c->recv_len -= message_end;

            } else {
                c->recv_len = 0;
            }

            return 0;
        }

        if (n == 0) {
            c->last_errno = ECONNRESET;
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK) {

            return 0;
        }

        c->last_errno = errno;
        return -1;
    }
}

static void close_client(Client *c)
{
    if (c->fd != -1) {
        close(c->fd);
        c->fd = -1;
    }

    c->state = CLIENT_DONE;
}

static void reset_client(Client *c)
{
    if (c->fd != -1) {
        close(c->fd);
    }

    memset(c, 0, sizeof(*c));

    c->fd = -1;
}

static int reconnect_client(
    Client *c,
    const char *host,
    int port
)
{
    reset_client(c);

    int fd = connect_nonblock(host, port);

    if (fd == -1) {
        c->state = CLIENT_FAILED;
        c->last_errno = errno;
        return -1;
    }

    c->fd = fd;
    c->state = CLIENT_CONNECTING;

    return 0;
}

static void print_progress(
    uint64_t completed,
    uint64_t total
)
{
    const int width = PROGRESS_WIDTH;

    double percent = 0.0;

    if (total > 0) {
        percent =
            (double)completed /
            (double)total *
            100.0;
    }

    if (percent > 100.0) {
        percent = 100.0;
    }

    int filled =
        (int)(percent / 100.0 * width);

    if (filled > width) {
        filled = width;
    }

    printf("\r[");

    for (int i = 0; i < width; i++) {
        putchar(i < filled ? '#' : '-');
    }

    printf("] %6.2f%%", percent);

    fflush(stdout);
}

int main(int argc, char **argv)
{
    const char *host =
        argc > 1
            ? argv[1]
            : DEFAULT_HOST;

    int port =
        argc > 2
            ? atoi(argv[2])
            : DEFAULT_PORT;

    int client_count =
        argc > 3
            ? atoi(argv[3])
            : DEFAULT_CLIENTS;

    uint64_t requests_per_client =
        argc > 4
            ? strtoull(argv[4], NULL, 10)
            : DEFAULT_REQUESTS;

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port.\n");
        return EXIT_FAILURE;
    }

    if (client_count <= 0) {
        fprintf(stderr, "Invalid client count.\n");
        return EXIT_FAILURE;
    }

    if (requests_per_client == 0) {
        fprintf(stderr, "Invalid request count.\n");
        return EXIT_FAILURE;
    }

    uint64_t total_requests =
        (uint64_t)client_count *
        requests_per_client;

    printf(
        "Target: %s:%d\n"
        "Clients: %d\n"
        "Requests per client: %llu\n"
        "Total requests: %llu\n\n",
        host,
        port,
        client_count,
        (unsigned long long)requests_per_client,
        (unsigned long long)total_requests
    );

    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);

    Client *clients =
        calloc(
            (size_t)client_count,
            sizeof(Client)
        );

    if (clients == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    uint64_t *next_sequence =
        calloc(
            (size_t)client_count,
            sizeof(uint64_t)
        );

    if (next_sequence == NULL) {
        perror("calloc");
        free(clients);
        return EXIT_FAILURE;
    }

    struct pollfd *pfds =
        calloc(
            (size_t)client_count,
            sizeof(struct pollfd)
        );

    if (pfds == NULL) {
        perror("calloc");
        free(next_sequence);
        free(clients);
        return EXIT_FAILURE;
    }

    Stats stats;

    stats_init(
        &stats,
        total_requests
    );

    /*
     * 建立全部并发连接。
     */
    for (int i = 0;
         i < client_count;
         i++) {

        clients[i].fd = -1;
        clients[i].id = i;

        if (reconnect_client(
                &clients[i],
                host,
                port
            ) == -1) {

            stats.failed++;
        }
    }

    uint64_t last_progress = 0;

    while (!stop_requested &&
           stats.completed + stats.failed <
               total_requests) {

        /*
         * 给 pollfd 数组建立与 Client
         * 一一对应的关系。
         */
        for (int i = 0;
             i < client_count;
             i++) {

            Client *c = &clients[i];

            pfds[i].fd = c->fd;
            pfds[i].events = 0;
            pfds[i].revents = 0;

            if (c->fd == -1) {
                continue;
            }

            switch (c->state) {

                case CLIENT_CONNECTING:
                    pfds[i].events =
                        POLLOUT |
                        POLLERR |
                        POLLHUP;
                    break;

                case CLIENT_WRITING:
                    pfds[i].events =
                        POLLOUT |
                        POLLERR |
                        POLLHUP;
                    break;

                case CLIENT_READING:
                    pfds[i].events =
                        POLLIN |
                        POLLERR |
                        POLLHUP;
                    break;

                case CLIENT_DONE:
                    /*
                     * 一个 request 完成。
                     * 准备下一次 request。
                     */
                    if (next_sequence[i] <
                        requests_per_client) {

                        if (prepare_request(
                                c,
                                next_sequence[i]
                            ) == -1) {

                            c->last_errno = errno;
                            stats.failed++;
                            close_client(c);
                            continue;
                        }

                        next_sequence[i]++;
                    }
                    break;

                case CLIENT_FAILED:
                    break;
            }
        }

        int ret = poll(
            pfds,
            (nfds_t)client_count,
            POLL_TIMEOUT_MS
        );

        if (ret == -1) {

            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            break;
        }

        /*
         * 处理每一个客户端。
         */
        for (int i = 0;
             i < client_count;
             i++) {

            Client *c = &clients[i];

            if (c->fd == -1) {
                continue;
            }

            short revents = pfds[i].revents;

            if (revents == 0) {
                continue;
            }

            /*
             * 建立连接完成。
             */
            if (c->state == CLIENT_CONNECTING) {

                if (revents &
                    (POLLOUT | POLLERR | POLLHUP)) {

                    if (handle_connect(c) == -1) {

                        stats.failed++;

                        close_client(c);
                        continue;
                    }

                    if (prepare_request(
                            c,
                            next_sequence[i]
                        ) == -1) {

                        stats.failed++;

                        close_client(c);
                        continue;
                    }

                    next_sequence[i]++;
                }

                continue;
            }

            /*
             * 致命事件。
             */
            if (revents &
                (POLLERR | POLLHUP | POLLNVAL)) {

                c->last_errno = ECONNRESET;

                stats.failed++;

                close_client(c);
                continue;
            }

            /*
             * 发送。
             */
            if (c->state == CLIENT_WRITING &&
                (revents & POLLOUT)) {

                if (handle_send(
                        c,
                        &stats
                    ) == -1) {

                    stats.failed++;

                    close_client(c);
                    continue;
                }
            }

            /*
             * 接收。
             */
            if (c->state == CLIENT_READING &&
                (revents & POLLIN)) {

                if (handle_recv(
                        c,
                        &stats
                    ) == -1) {

                    stats.failed++;

                    close_client(c);
                    continue;
                }
            }

            /*
             * 一个 request 已经完成。
             */
            if (c->state == CLIENT_DONE) {

                if (next_sequence[i] <
                    requests_per_client) {

                    if (prepare_request(
                            c,
                            next_sequence[i]
                        ) == -1) {

                        stats.failed++;

                        close_client(c);
                        continue;
                    }

                    next_sequence[i]++;
                } else {

                    /*
                     * 这个客户端已经把自己的
                     * 全部 request 做完。
                     */
                    close_client(c);
                }
            }
        }

        /*
         * 控制台刷新。
         *
         * 不做复杂 UI，
         * 就一根进度条。
         */
        if (stats.completed != last_progress) {

            print_progress(
                stats.completed,
                total_requests
            );

            last_progress = stats.completed;
        }
    }

    /*
     * 最终输出。
     */
    if (stop_requested) {
        printf("\n\nInterrupted.\n");
    } else {
        print_progress(
            stats.completed,
            total_requests
        );

        printf("\n\n");
    }

    double elapsed =
        elapsed_seconds(stats.start_time);

    if (elapsed <= 0.0) {
        elapsed = 1e-9;
    }

    double rps =
        (double)stats.completed /
        elapsed;

    double throughput =
        (double)stats.bytes_received /
        elapsed /
        1024.0 /
        1024.0;

    double avg_ms = 0.0;

    if (stats.completed > 0) {
        avg_ms =
            ((double)stats.latency_sum_ns /
             (double)stats.completed) /
            1000000.0;
    }

    double p50 =
        percentile_ms(&stats, 50.0);

    double p95 =
        percentile_ms(&stats, 95.0);

    double p99 =
        percentile_ms(&stats, 99.0);

    printf(
        "Completed:  %llu\n"
        "Failed:     %llu\n"
        "RPS:        %.2f\n"
        "Throughput: %.2f MiB/s\n"
        "Avg RTT:    %.3f ms\n"
        "P50 RTT:    %.3f ms\n"
        "P95 RTT:    %.3f ms\n"
        "P99 RTT:    %.3f ms\n"
        "Sent:       %llu bytes\n"
        "Received:   %llu bytes\n"
        "Elapsed:    %.3f s\n",
        (unsigned long long)stats.completed,
        (unsigned long long)stats.failed,
        rps,
        throughput,
        avg_ms,
        p50,
        p95,
        p99,
        (unsigned long long)stats.bytes_sent,
        (unsigned long long)stats.bytes_received,
        elapsed
    );

    for (int i = 0;
         i < client_count;
         i++) {

        close_client(&clients[i]);
    }

    stats_destroy(&stats);

    free(pfds);
    free(next_sequence);
    free(clients);

    return stats.failed == 0 &&
           !stop_requested
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}