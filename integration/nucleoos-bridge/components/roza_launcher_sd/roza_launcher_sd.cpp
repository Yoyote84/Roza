// Read-only SD firmware preflight for ROZA.
// NucleoOS remains the sole SD owner: this app uses its mounted VFS at NUCLEO_SD_MOUNT
// and opens candidate files read-only. It never mounts, formats, writes or flashes anything.
#include "nucleo_app.h"
#include "app_gfx.h"
#include "launcher_theme.h"
#include "nucleo_storage.h"
#include "nucleo_board.h"

#include "esp_app_format.h"
#include "esp_image_format.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <stdint.h>
#include <limits.h>

namespace {

constexpr int kMaxEntries = 64;
constexpr size_t kNameSize = 64;
constexpr size_t kPathSize = 192;
constexpr uint32_t kPartitionTableOffset = 0x8000;
constexpr uint32_t kPartitionEntrySize = 32;
constexpr int kMaxPartitionEntries = 96;

struct SdEntry {
    char name[kNameSize];
    bool directory;
    uint32_t size;
};

struct FirmwareInfo {
    bool parsed;
    bool full_image;
    uint32_t file_size;
    uint32_t image_offset;
    uint32_t image_size;
    uint32_t declared_partition_size;
    uint8_t segments;
    uint16_t chip_id;
    int partition_count;
    char app_label[17];
    char message[64];
};

static SdEntry *s_entries;
static int s_count;
static int s_selected;
static int s_first;
static int s_visible = 4;
static char s_path[kPathSize] = "/";
static char s_status[64];
static bool s_details_open;
static char s_selected_path[320];
static FirmwareInfo s_info;

static bool is_bin(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && strcasecmp(dot, ".bin") == 0;
}

static int entry_compare(const void *left, const void *right)
{
    const SdEntry *a = static_cast<const SdEntry *>(left);
    const SdEntry *b = static_cast<const SdEntry *>(right);
    if (a->directory != b->directory) return a->directory ? -1 : 1;
    return strcasecmp(a->name, b->name);
}

static bool storage_ready()
{
    const nucleo_storage_info_t *info = nucleo_storage_info();
    return info && info->mounted;
}

static void scan_directory()
{
    s_count = 0;
    s_selected = 0;
    s_first = 0;
    s_status[0] = '\0';

    if (!s_entries) {
        snprintf(s_status, sizeof(s_status), "RAM insuffisante");
        return;
    }
    if (!storage_ready()) {
        snprintf(s_status, sizeof(s_status), "Carte SD non montee");
        return;
    }

    char absolute_dir[320];
    if (snprintf(absolute_dir, sizeof(absolute_dir), "%s%s", NUCLEO_SD_MOUNT, s_path) >=
        static_cast<int>(sizeof(absolute_dir))) {
        snprintf(s_status, sizeof(s_status), "Chemin trop long");
        return;
    }

    DIR *dir = opendir(absolute_dir);
    if (!dir) {
        snprintf(s_status, sizeof(s_status), "Ouverture SD impossible");
        return;
    }

    struct dirent *item;
    while ((item = readdir(dir)) != nullptr && s_count < kMaxEntries) {
        if (!item->d_name[0] || item->d_name[0] == '.') continue;

        char full_path[384];
        if (snprintf(full_path, sizeof(full_path), "%s%s", absolute_dir, item->d_name) >=
            static_cast<int>(sizeof(full_path))) continue;

        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        const bool is_directory = S_ISDIR(st.st_mode);
        if (!is_directory && !is_bin(item->d_name)) continue;

        SdEntry &entry = s_entries[s_count];
        if (snprintf(entry.name, sizeof(entry.name), "%s", item->d_name) >=
            static_cast<int>(sizeof(entry.name))) continue;
        entry.directory = is_directory;
        entry.size = is_directory || st.st_size < 0 ? 0 :
            (st.st_size > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(st.st_size));
        ++s_count;
    }
    closedir(dir);
    qsort(s_entries, static_cast<size_t>(s_count), sizeof(SdEntry), entry_compare);

    if (s_count == 0) snprintf(s_status, sizeof(s_status), "Aucun .bin dans ce dossier");
    else snprintf(s_status, sizeof(s_status), "%d element(s) compatible(s)", s_count);
}

static void go_up()
{
    if (strcmp(s_path, "/") == 0) return;
    size_t length = strlen(s_path);
    if (length > 1 && s_path[length - 1] == '/') s_path[length - 1] = '\0';
    char *slash = strrchr(s_path, '/');
    if (slash == s_path) s_path[1] = '\0';
    else if (slash) slash[1] = '\0';
    else snprintf(s_path, sizeof(s_path), "/");
    scan_directory();
}

static bool read_at(FILE *file, uint32_t offset, void *buffer, size_t length)
{
    if (!file || !buffer || offset > static_cast<uint32_t>(LONG_MAX)) return false;
    if (fseek(file, static_cast<long>(offset), SEEK_SET) != 0) return false;
    return fread(buffer, 1, length, file) == length;
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

static uint32_t align16(uint32_t value)
{
    return (value + 15u) & ~15u;
}

// This checks image framing and segment boundaries only. It deliberately does not claim
// SHA-256, signature, Secure Boot or anti-rollback verification.
static bool measure_image(FILE *file, uint32_t file_size, uint32_t offset,
                          uint32_t available, uint32_t *measured, uint8_t *segments_out,
                          uint16_t *chip_id_out, char *reason, size_t reason_size)
{
    esp_image_header_t header = {};
    if (!read_at(file, offset, &header, sizeof(header))) {
        snprintf(reason, reason_size, "Entete incomplet");
        return false;
    }
    if (header.magic != ESP_IMAGE_HEADER_MAGIC) {
        snprintf(reason, reason_size, "Magic ESP invalide");
        return false;
    }
    if (header.segment_count == 0 || header.segment_count > ESP_IMAGE_MAX_SEGMENTS) {
        snprintf(reason, reason_size, "Nombre de segments invalide");
        return false;
    }

    uint64_t cursor = static_cast<uint64_t>(offset) + sizeof(header);
    const uint64_t limit = static_cast<uint64_t>(offset) + available;
    if (limit > file_size || cursor > limit) {
        snprintf(reason, reason_size, "Image hors limites");
        return false;
    }

    for (uint8_t i = 0; i < header.segment_count; ++i) {
        uint8_t segment_header[sizeof(esp_image_segment_header_t)] = {};
        if (cursor > UINT32_MAX ||
            !read_at(file, static_cast<uint32_t>(cursor), segment_header, sizeof(segment_header))) {
            snprintf(reason, reason_size, "Segment %u incomplet", static_cast<unsigned>(i + 1));
            return false;
        }
        const uint32_t segment_size = read_le32(segment_header + 4);
        cursor += sizeof(segment_header);
        if (cursor > limit || static_cast<uint64_t>(segment_size) > limit - cursor) {
            snprintf(reason, reason_size, "Segment %u hors limites", static_cast<unsigned>(i + 1));
            return false;
        }
        cursor += segment_size;
    }

    if (cursor > UINT32_MAX - 16u) {
        snprintf(reason, reason_size, "Image trop grande");
        return false;
    }
    uint64_t end = static_cast<uint64_t>(align16(static_cast<uint32_t>(cursor))) + 1u;
    if (header.hash_appended) end += ESP_IMAGE_HASH_LEN;
    end = (end + 15u) & ~static_cast<uint64_t>(15u);
    if (end <= offset || end > limit || end > file_size) {
        snprintf(reason, reason_size, "Fin d'image hors limites");
        return false;
    }

    *measured = static_cast<uint32_t>(end - offset);
    *segments_out = header.segment_count;
    *chip_id_out = static_cast<uint16_t>(header.chip_id);
    snprintf(reason, reason_size, "Structure ESP-IDF plausible");
    return true;
}

static bool analyze_firmware(const char *path, FirmwareInfo *out)
{
    if (!path || !out) return false;
    memset(out, 0, sizeof(*out));

    FILE *file = fopen(path, "rb");
    if (!file) {
        snprintf(out->message, sizeof(out->message), "Lecture impossible");
        return false;
    }
    bool ok = false;
    if (fseek(file, 0, SEEK_END) != 0) {
        snprintf(out->message, sizeof(out->message), "Taille illisible");
        goto done;
    }

    {
        const long end = ftell(file);
        if (end <= 0 || static_cast<unsigned long>(end) > UINT32_MAX) {
            snprintf(out->message, sizeof(out->message), "Taille de fichier invalide");
            goto done;
        }
        out->file_size = static_cast<uint32_t>(end);
    }

    {
        uint8_t first_entry[3] = {};
        const bool has_table = out->file_size >= kPartitionTableOffset + sizeof(first_entry) &&
            read_at(file, kPartitionTableOffset, first_entry, sizeof(first_entry)) &&
            first_entry[0] == 0xAA && first_entry[1] == 0x50 &&
            (first_entry[2] == 0x00 || first_entry[2] == 0x01);

        uint32_t image_offset = 0;
        uint32_t available = out->file_size;
        if (has_table) {
            out->full_image = true;
            bool found_app = false;
            for (int i = 0; i < kMaxPartitionEntries; ++i) {
                uint8_t entry[kPartitionEntrySize] = {};
                const uint32_t entry_offset = kPartitionTableOffset +
                    static_cast<uint32_t>(i) * kPartitionEntrySize;
                if (!read_at(file, entry_offset, entry, sizeof(entry))) {
                    snprintf(out->message, sizeof(out->message), "Table partitions incomplete");
                    goto done;
                }
                if ((entry[0] == 0xEB && entry[1] == 0xEB) ||
                    (entry[0] == 0xFF && entry[1] == 0xFF)) break;

                ++out->partition_count;
                const uint8_t type = entry[2];
                const uint8_t subtype = entry[3];
                if (!found_app && type == 0x00 &&
                    (subtype == 0x00 || (subtype >= 0x10 && subtype <= 0x1F) || subtype == 0x20)) {
                    image_offset = read_le32(entry + 4);
                    out->declared_partition_size = read_le32(entry + 8);
                    memcpy(out->app_label, entry + 12, 16);
                    out->app_label[16] = '\0';
                    found_app = true;
                }
            }

            if (!found_app) {
                snprintf(out->message, sizeof(out->message), "Aucune partition APP");
                goto done;
            }
            if (image_offset >= out->file_size) {
                snprintf(out->message, sizeof(out->message), "Offset APP hors fichier");
                goto done;
            }
            const uint32_t remaining = out->file_size - image_offset;
            available = out->declared_partition_size == 0 ? remaining :
                (out->declared_partition_size < remaining ? out->declared_partition_size : remaining);
        }

        out->image_offset = image_offset;
        if (!measure_image(file, out->file_size, image_offset, available,
                           &out->image_size, &out->segments, &out->chip_id,
                           out->message, sizeof(out->message))) {
            goto done;
        }
        out->parsed = true;
        ok = true;
    }

done:
    fclose(file);
    return ok;
}

static void select_entry()
{
    if (s_selected < 0 || s_selected >= s_count) return;
    const SdEntry &entry = s_entries[s_selected];

    if (entry.directory) {
        const size_t length = strlen(s_path);
        const int written = snprintf(s_path + length, sizeof(s_path) - length, "%s/",
                                     entry.name);
        if (written < 0 || static_cast<size_t>(written) >= sizeof(s_path) - length) {
            snprintf(s_status, sizeof(s_status), "Chemin trop long");
            return;
        }
        scan_directory();
        return;
    }

    if (snprintf(s_selected_path, sizeof(s_selected_path), "%s%s%s",
                 NUCLEO_SD_MOUNT, s_path, entry.name) >= static_cast<int>(sizeof(s_selected_path))) {
        snprintf(s_status, sizeof(s_status), "Chemin trop long");
        return;
    }
    analyze_firmware(s_selected_path, &s_info);
    s_details_open = true;
    nucleo_app_set_hint("Touche = retour a la liste");
}

static bool sd_browser_back(int key)
{
    (void)key;
    if (s_details_open) {
        s_details_open = false;
        nucleo_app_set_hint("haut/bas naviguer  entree analyser  esc retour");
        nucleo_app_request_draw();
        return true;
    }
    if (strcmp(s_path, "/") != 0) {
        go_up();
        nucleo_app_request_draw();
        return true;
    }
    return false;
}

static void sd_browser_enter()
{
    if (!s_entries) s_entries = static_cast<SdEntry *>(calloc(kMaxEntries, sizeof(SdEntry)));
    s_details_open = false;
    snprintf(s_path, sizeof(s_path), "/");
    nucleo_app_set_back_handler(sd_browser_back);
    nucleo_app_set_hint("haut/bas naviguer  entree analyser  esc retour");
    scan_directory();
    nucleo_app_request_draw();
}

static void sd_browser_key(int key, char ch)
{
    (void)ch;
    if (s_details_open) {
        s_details_open = false;
        nucleo_app_set_hint("haut/bas naviguer  entree analyser  esc retour");
        nucleo_app_request_draw();
        return;
    }

    if (key == NK_UP && s_selected > 0) {
        --s_selected;
        if (s_selected < s_first) s_first = s_selected;
        nucleo_app_request_draw();
    } else if (key == NK_DOWN && s_selected + 1 < s_count) {
        ++s_selected;
        if (s_selected >= s_first + s_visible) s_first = s_selected - s_visible + 1;
        nucleo_app_request_draw();
    } else if (key == NK_ENTER) {
        select_entry();
        nucleo_app_request_draw();
    }
}

static void draw_details(int top, int height)
{
    d.setFont(&fonts::Font0);
    d.setTextSize(1);
    d.setTextColor(C_BLUE, BG);
    d.setCursor(8, top + 2);
    d.print(s_info.full_image ? "IMAGE COMPLETE" : "APPLICATION ESP-IDF");
    d.drawFastHLine(8, top + 14, W - 16, LINE);

    int y = top + 19;
    char line[64];
    d.setTextColor(FG, BG);
    d.setCursor(8, y);
    d.print(s_info.message);
    y += 13;

    if (!s_info.parsed) return;
    snprintf(line, sizeof(line), "Fichier: %lu KB",
             static_cast<unsigned long>(s_info.file_size / 1024u));
    d.setCursor(8, y); d.print(line); y += 12;
    snprintf(line, sizeof(line), "Image: %lu KB a 0x%06lX",
             static_cast<unsigned long>(s_info.image_size / 1024u),
             static_cast<unsigned long>(s_info.image_offset));
    d.setCursor(8, y); d.print(line); y += 12;
    snprintf(line, sizeof(line), "%u segment(s), puce 0x%04X",
             static_cast<unsigned>(s_info.segments), s_info.chip_id);
    d.setCursor(8, y); d.print(line); y += 12;
    if (s_info.full_image) {
        snprintf(line, sizeof(line), "Partition: %.16s (%d entree(s))",
                 s_info.app_label, s_info.partition_count);
        d.setCursor(8, y); d.print(line);
    } else {
        d.setCursor(8, y); d.print("Partition table: non incluse");
    }
    if (height > 0) {
        d.setTextColor(MUTED, BG);
        d.setCursor(8, top + height - 11);
        d.print("Controle structurel seulement");
    }
}

static void sd_browser_draw()
{
    const int top = nucleo_app_content_top();
    const int height = nucleo_app_content_height();
    d.setFont(&fonts::Font0);
    d.setTextSize(1);
    d.fillRect(0, top, W, height, BG);

    if (s_details_open) {
        draw_details(top, height);
        return;
    }

    d.setTextColor(C_BLUE, BG);
    d.setCursor(8, top + 2);
    d.print("FIRMWARE SD");
    d.setTextColor(MUTED, BG);
    d.setCursor(92, top + 2);
    d.print(s_path);
    d.drawFastHLine(8, top + 14, W - 16, LINE);

    const int row_top = top + 17;
    const int row_height = 19;
    s_visible = (height - 18) / row_height;
    if (s_visible < 1) s_visible = 1;
    if (s_first > s_count - s_visible) s_first = s_count > s_visible ? s_count - s_visible : 0;

    if (!storage_ready()) {
        d.setTextColor(C_RED, BG);
        d.setCursor(8, row_top + 5);
        d.print("Carte SD non montee par NucleoOS");
        return;
    }
    if (s_count == 0) {
        d.setTextColor(MUTED, BG);
        d.setCursor(8, row_top + 5);
        d.print(s_status[0] ? s_status : "Aucun fichier .bin");
        return;
    }

    for (int slot = 0; slot < s_visible; ++slot) {
        const int index = s_first + slot;
        if (index >= s_count) break;
        const SdEntry &entry = s_entries[index];
        const int y = row_top + slot * row_height;
        const bool selected = index == s_selected;
        const uint16_t background = selected ? THEME_SEL : BG;
        d.fillRect(6, y, W - 12, row_height - 1, background);
        d.setTextColor(selected ? INK : (entry.directory ? C_YELLOW : FG), background);
        d.setCursor(10, y + 5);
        d.print(entry.directory ? "[DIR]" : "[BIN]");
        d.setCursor(48, y + 5);
        d.print(entry.name);
        if (!entry.directory) {
            char size[12];
            snprintf(size, sizeof(size), "%luK", static_cast<unsigned long>(entry.size / 1024u));
            d.setTextColor(selected ? INK : MUTED, background);
            d.setCursor(W - 43, y + 5);
            d.print(size);
        }
    }
}

static void sd_browser_exit()
{
    free(s_entries);
    s_entries = nullptr;
    s_count = 0;
    s_selected = 0;
    s_first = 0;
    s_details_open = false;
    s_selected_path[0] = '\0';
}

} // namespace

extern "C" void roza_register_launcher_sd(void)
{
    static const nucleo_app_def_t app = {
        "roza-firmware-sd",
        "Firmware SD",
        "System",
        "Inspecteur .bin en lecture seule",
        'B',
        C_BLUE,
        sd_browser_enter,
        sd_browser_key,
        nullptr,
        sd_browser_draw,
        sd_browser_exit,
        0,
        nullptr,
        nullptr
    };
    nucleo_app_register(&app);
}
