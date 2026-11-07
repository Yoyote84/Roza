// First native Launcher feature in ROZA: a safe, read-only view of the live ESP-IDF
// partition table. It deliberately does not write, erase, compact, migrate or change boot slots.
#include "nucleo_app.h"
#include "app_gfx.h"
#include "launcher_theme.h"

#include "esp_flash.h"
#include "esp_partition.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

struct PartitionRow {
    char label[17];
    uint32_t address;
    uint32_t size;
    uint8_t type;
    uint8_t subtype;
    bool encrypted;
};

static PartitionRow *s_rows;
static int s_count;
static int s_first;
static int s_visible = 4;
static uint32_t s_flash_size;
static char s_error[48];

static int count_partitions()
{
    int count = 0;
    esp_partition_iterator_t it =
        esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (it != nullptr) {
        ++count;
        it = esp_partition_next(it);
    }
    return count;
}

static bool read_partitions()
{
    uint32_t flash_size = 0;
    const bool have_flash_size = esp_flash_get_size(nullptr, &flash_size) == ESP_OK;
    const int expected = count_partitions();

    if (expected < 0) {
        snprintf(s_error, sizeof(s_error), "Lecture impossible");
        return false;
    }

    PartitionRow *rows = nullptr;
    if (expected > 0) {
        rows = static_cast<PartitionRow *>(calloc(static_cast<size_t>(expected), sizeof(*rows)));
        if (!rows) {
            snprintf(s_error, sizeof(s_error), "RAM insuffisante");
            return false;
        }
    }

    int loaded = 0;
    esp_partition_iterator_t it =
        esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (it != nullptr && loaded < expected) {
        const esp_partition_t *partition = esp_partition_get(it);
        if (partition != nullptr) {
            PartitionRow &row = rows[loaded++];
            snprintf(row.label, sizeof(row.label), "%s", partition->label);
            row.address = partition->address;
            row.size = partition->size;
            row.type = static_cast<uint8_t>(partition->type);
            row.subtype = static_cast<uint8_t>(partition->subtype);
            row.encrypted = partition->encrypted;
        }
        it = esp_partition_next(it);
    }

    free(s_rows);
    s_rows = rows;
    s_count = loaded;
    s_first = 0;
    s_flash_size = have_flash_size ? flash_size : 0;
    s_error[0] = '\0';
    return true;
}

static const char *type_name(uint8_t type)
{
    if (type == ESP_PARTITION_TYPE_APP) return "APP";
    if (type == ESP_PARTITION_TYPE_DATA) return "DATA";
    return "AUTRE";
}

static uint16_t type_color(uint8_t type)
{
    if (type == ESP_PARTITION_TYPE_APP) return C_BLUE;
    if (type == ESP_PARTITION_TYPE_DATA) return C_GREEN;
    return C_YELLOW;
}

static void partitions_enter()
{
    read_partitions();
    nucleo_app_set_hint("haut/bas defiler  r relire  esc retour");
    nucleo_app_request_draw();
}

static void partitions_key(int key, char ch)
{
    if (ch == 'r' || ch == 'R') {
        read_partitions();
        nucleo_app_request_draw();
        return;
    }

    if (key == NK_UP && s_first > 0) {
        --s_first;
        nucleo_app_request_draw();
    } else if (key == NK_DOWN && s_first + s_visible < s_count) {
        ++s_first;
        nucleo_app_request_draw();
    }
}

static void partitions_draw()
{
    const int top = nucleo_app_content_top();
    const int height = nucleo_app_content_height();

    d.setFont(&fonts::Font0);
    d.setTextSize(1);
    d.fillRect(0, top, W, height, BG);

    char heading[40];
    if (s_flash_size > 0) {
        snprintf(heading, sizeof(heading), "FLASH %lu MB  |  %d PARTITIONS",
                 static_cast<unsigned long>(s_flash_size / (1024u * 1024u)), s_count);
    } else {
        snprintf(heading, sizeof(heading), "FLASH ?  |  %d PARTITIONS", s_count);
    }
    d.setTextColor(C_BLUE, BG);
    d.setCursor(8, top + 1);
    d.print(heading);
    d.drawFastHLine(8, top + 14, W - 16, LINE);

    const int row_top = top + 17;
    const int row_height = 21;
    s_visible = (height - 17) / row_height;
    if (s_visible < 1) s_visible = 1;

    if (s_count == 0) {
        d.setTextColor(s_error[0] ? C_RED : MUTED, BG);
        d.setCursor(8, row_top + 4);
        d.print(s_error[0] ? s_error : "Aucune partition trouvee");
        return;
    }

    const int max_first = s_count > s_visible ? s_count - s_visible : 0;
    if (s_first > max_first) s_first = max_first;

    for (int slot = 0; slot < s_visible; ++slot) {
        const int index = s_first + slot;
        if (index >= s_count) break;

        const PartitionRow &row = s_rows[index];
        const int y = row_top + slot * row_height;
        d.setTextColor(FG, BG);
        d.setCursor(8, y);
        d.print(row.label);

        char kind[12];
        snprintf(kind, sizeof(kind), "%s %02X", type_name(row.type), row.subtype);
        d.setTextColor(type_color(row.type), BG);
        d.setCursor(158, y);
        d.print(kind);

        char details[40];
        snprintf(details, sizeof(details), "0x%06lX  %lu KB%s",
                 static_cast<unsigned long>(row.address),
                 static_cast<unsigned long>(row.size / 1024u),
                 row.encrypted ? " ENC" : "");
        d.setTextColor(MUTED, BG);
        d.setCursor(8, y + 9);
        d.print(details);
        d.drawFastHLine(8, y + 20, W - 16, LINE);
    }

    if (s_count > s_visible) {
        const int track_height = height - 22;
        const int thumb_height = track_height * s_visible / s_count;
        const int travel = track_height - thumb_height;
        const int thumb_y = row_top + (max_first > 0 ? travel * s_first / max_first : 0);
        d.fillRect(W - 3, row_top, 2, track_height, LINE);
        d.fillRect(W - 3, thumb_y, 2, thumb_height, C_BLUE);
    }
}

static void partitions_exit()
{
    free(s_rows);
    s_rows = nullptr;
    s_count = 0;
    s_first = 0;
    s_flash_size = 0;
    s_error[0] = '\0';
}

} // namespace

extern "C" void roza_register_launcher_partitions(void)
{
    static const nucleo_app_def_t app = {
        "roza-partitions",
        "Partitions",
        "System",
        "Table flash en lecture seule",
        'P',
        C_BLUE,
        partitions_enter,
        partitions_key,
        nullptr,
        partitions_draw,
        partitions_exit,
        0,
        nullptr,
        nullptr
    };
    nucleo_app_register(&app);
}
