#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned char *data;
    size_t len;
} Buffer;

static size_t env_size(const char *key, size_t default_mb) {
    const char *v = getenv(key);
    if (!v) return default_mb;
    char *end = NULL;
    long parsed = strtol(v, &end, 10);
    if (end == v || parsed <= 0) return default_mb;
    return (size_t)parsed;
}

int main(void) {
    size_t chunk_mb = env_size("LEAK_CHUNK_MB", 10);
    unsigned long pause_ms = env_size("LEAK_PAUSE_MS", 500);

    printf("slow_leak: 每次保留 %zu MB，间隔 %lu ms；总占用会持续升高\n",
           chunk_mb, pause_ms);

    Buffer *store = NULL;
    size_t store_len = 0;
    size_t store_cap = 0;
    size_t total_bytes = 0;
    size_t bytes = chunk_mb * 1024 * 1024;

    for (;;) {
        if (store_len == store_cap) {
            size_t new_cap = store_cap ? store_cap * 2 : 16;
            Buffer *new_store = realloc(store, new_cap * sizeof(Buffer));
            if (!new_store) {
                fprintf(stderr, "扩容失败，终止。\n");
                free(store);
                return 1;
            }
            store = new_store;
            store_cap = new_cap;
        }

        unsigned char *buf = malloc(bytes);
        if (!buf) {
            fprintf(stderr, "分配失败，终止。\n");
            free(store);
            return 1;
        }
        memset(buf, 0x2A, bytes);
        store[store_len++] = (Buffer){.data = buf, .len = bytes};
        total_bytes += bytes;

        if ((total_bytes / (1024 * 1024)) % 100 == 0) {
            printf("当前累计保留约 %zu MB\n", total_bytes / (1024 * 1024));
        }

        usleep(pause_ms * 1000);
    }
}
