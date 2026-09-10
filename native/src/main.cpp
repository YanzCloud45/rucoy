#include "zygisk.hpp"
#include "dobby.h"
#include "config.hpp"

#include <android/log.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LOG_TAG "RucoyTileBot"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr uint32_t kTapMagic = 0x52425450; // RBTP
constexpr size_t kStateSlots = 512;
constexpr size_t kProbeCount = 8;

struct TapRequest {
    uint32_t magic;
    int32_t x;
    int32_t y;
    int32_t display_id;
};

struct MonsterState {
    uintptr_t ptr;
    bool inside;
};

MonsterState g_states[kStateSlots] = {};
std::atomic<int> g_pending_taps{0};
std::atomic<bool> g_hook_ready{false};
int g_companion_fd = -1;

bool write_full(int fd, const void *buf, size_t size) {
    const auto *p = static_cast<const uint8_t *>(buf);
    size_t done = 0;
    while (done < size) {
        const ssize_t n = TEMP_FAILURE_RETRY(write(fd, p + done, size - done));
        if (n <= 0) return false;
        done += static_cast<size_t>(n);
    }
    return true;
}

bool read_full(int fd, void *buf, size_t size) {
    auto *p = static_cast<uint8_t *>(buf);
    size_t done = 0;
    while (done < size) {
        const ssize_t n = TEMP_FAILURE_RETRY(read(fd, p + done, size - done));
        if (n <= 0) return false;
        done += static_cast<size_t>(n);
    }
    return true;
}

void perform_root_tap(const TapRequest &req) {
    char x[16];
    char y[16];
    char display[16];
    std::snprintf(x, sizeof(x), "%d", req.x);
    std::snprintf(y, sizeof(y), "%d", req.y);
    std::snprintf(display, sizeof(display), "%d", req.display_id);

    const pid_t pid = fork();
    if (pid == 0) {
        execl("/system/bin/input", "input", "-d", display, "tap", x, y,
              static_cast<char *>(nullptr));
        _exit(127);
    }
    if (pid > 0) {
        int status = 0;
        TEMP_FAILURE_RETRY(waitpid(pid, &status, 0));
    }
}

void companion_handler(int client) {
    LOGI("root companion connected");
    for (;;) {
        TapRequest req{};
        if (!read_full(client, &req, sizeof(req))) break;
        if (req.magic != kTapMagic) continue;
        perform_root_tap(req);
        LOGI("root tap display=%d x=%d y=%d", req.display_id, req.x, req.y);
    }
    close(client);
    LOGI("root companion disconnected");
}

uintptr_t find_library_base(const char *needle) {
    FILE *fp = std::fopen("/proc/self/maps", "r");
    if (!fp) return 0;

    char line[1024];
    uintptr_t base = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        if (!std::strstr(line, needle)) continue;

        unsigned long start = 0;
        unsigned long end = 0;
        unsigned long offset = 0;
        char perms[8] = {};
        if (std::sscanf(line, "%lx-%lx %7s %lx", &start, &end, perms, &offset) >= 4) {
            base = static_cast<uintptr_t>(start - offset);
            break;
        }
    }

    std::fclose(fp);
    return base;
}

MonsterState *state_for(uintptr_t ptr) {
    const size_t start = static_cast<size_t>((ptr >> 4U) & (kStateSlots - 1U));

    for (size_t i = 0; i < kProbeCount; ++i) {
        MonsterState &slot = g_states[(start + i) & (kStateSlots - 1U)];
        if (slot.ptr == ptr) return &slot;
        if (slot.ptr == 0) {
            slot.ptr = ptr;
            slot.inside = false;
            return &slot;
        }
    }

    // Bounded table: recycle the home slot if the local probe window is full.
    MonsterState &slot = g_states[start];
    slot.ptr = ptr;
    slot.inside = false;
    return &slot;
}

inline bool near(float a, float b) {
    return std::fabs(a - b) <= cfg::kEpsilon;
}

void monster_probe(void *, DobbyRegisterContext *ctx) {
#if defined(__aarch64__)
    if (!ctx) return;

    const uintptr_t monster = static_cast<uintptr_t>(ctx->general.regs.x0);
    if (monster == 0) return;

    // ARM64 S0/S1 are the low float lane of Q0/Q1.
    const float x = ctx->floating.regs.q0.f.f1;
    const float y = ctx->floating.regs.q1.f.f1;

    if (!std::isfinite(x) || !std::isfinite(y)) return;
    if (x < 0.0f || x > 10000.0f || y < 0.0f || y > 10000.0f) return;

    const bool hit = near(x, cfg::kTargetX) && near(y, cfg::kTargetY);
    MonsterState *state = state_for(monster);
    if (!state) return;

    if (hit && !state->inside) {
        state->inside = true;
        g_pending_taps.fetch_add(1, std::memory_order_relaxed);
        LOGI("ENTER ptr=%p world=(%.3f,%.3f)", reinterpret_cast<void *>(monster), x, y);
    } else if (!hit && state->inside) {
        state->inside = false;
        LOGI("EXIT ptr=%p world=(%.3f,%.3f)", reinterpret_cast<void *>(monster), x, y);
    }
#else
    (void)ctx;
#endif
}

bool send_tap() {
    if (g_companion_fd < 0) return false;
    const TapRequest req{
        kTapMagic,
        cfg::kTapX,
        cfg::kTapY,
        cfg::kDisplayId,
    };
    return write_full(g_companion_fd, &req, sizeof(req));
}

void *worker_thread(void *) {
    uintptr_t base = 0;
    for (int i = 0; i < cfg::kLibraryWaitTries; ++i) {
        base = find_library_base(cfg::kTargetLibrary);
        if (base != 0) break;
        usleep(cfg::kLibraryWaitMs * 1000);
    }

    if (base == 0) {
        LOGE("%s not found; hook not installed", cfg::kTargetLibrary);
        return nullptr;
    }

    void *hook_address = reinterpret_cast<void *>(base + cfg::kHookRva);
    LOGI("%s base=%p hook=%p", cfg::kTargetLibrary,
         reinterpret_cast<void *>(base), hook_address);

    dobby_set_near_trampoline(true);
    const int rc = DobbyInstrument(hook_address, monster_probe);
    if (rc != 0) {
        LOGE("DobbyInstrument failed rc=%d", rc);
        return nullptr;
    }

    g_hook_ready.store(true, std::memory_order_release);
    LOGI("hook active target=(%.3f,%.3f) eps=%.3f tap=(%d,%d)",
         cfg::kTargetX, cfg::kTargetY, cfg::kEpsilon, cfg::kTapX, cfg::kTapY);

    for (;;) {
        const int pending = g_pending_taps.exchange(0, std::memory_order_acq_rel);
        if (pending > 0) {
            // Coalesce simultaneous ENTERs to one input event.
            if (!send_tap()) {
                LOGE("failed sending tap request to root companion");
            }
        }
        usleep(20 * 1000);
    }
}

class RucoyTileBot : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        target_ = false;
        if (!args || !args->nice_name) return;

        const char *name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (name) {
            target_ = std::strcmp(name, cfg::kTargetPackage) == 0;
            env_->ReleaseStringUTFChars(args->nice_name, name);
        }

        if (!target_) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        g_companion_fd = api_->connectCompanion();
        if (g_companion_fd < 0) {
            LOGE("connectCompanion failed");
            return;
        }

        if (!api_->exemptFd(g_companion_fd)) {
            LOGE("exemptFd failed; companion socket may be closed on specialize");
        }
        LOGI("target process selected; companion fd=%d", g_companion_fd);
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *) override {
        if (!target_) return;

        pthread_t thread{};
        const int rc = pthread_create(&thread, nullptr, worker_thread, nullptr);
        if (rc != 0) {
            LOGE("pthread_create failed rc=%d", rc);
            return;
        }
        pthread_detach(thread);
    }

private:
    zygisk::Api *api_ = nullptr;
    JNIEnv *env_ = nullptr;
    bool target_ = false;
};

} // namespace

REGISTER_ZYGISK_MODULE(RucoyTileBot)
REGISTER_ZYGISK_COMPANION(companion_handler)
