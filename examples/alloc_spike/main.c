#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static size_t env_size(const char *key, size_t default_mb) {
    const char *v = getenv(key);
    if (!v) return default_mb;
    char *end = NULL;
    long parsed = strtol(v, &end, 10);
    if (end == v || parsed <= 0) return default_mb;
    return (size_t)parsed;
}

int main(void) {
    size_t chunk_mb = env_size("ALLOC_CHUNK_MB", 200);
    unsigned long hold_ms = env_size("HOLD_MS", 1500);
    unsigned long pause_ms = env_size("PAUSE_MS", 1500);

    printf("alloc_spike: 每次分配约 %zu MB，保持 %lu ms 后释放，间隔 %lu ms 再次分配\n",
           chunk_mb, hold_ms, pause_ms);

    size_t bytes = chunk_mb * 1024 * 1024;

    for (;;) {
        void *buf = malloc(bytes);
        if (!buf) {
            fprintf(stderr, "分配失败，终止。\n");
            return 1;
        }
        memset(buf, 0, bytes);
        usleep(hold_ms * 1000);
        free(buf); // 释放，制造周期性峰值
        usleep(pause_ms * 1000);
    }
}
