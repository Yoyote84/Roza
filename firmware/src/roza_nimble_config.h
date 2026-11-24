#pragma once

// Harmonise la taille des événements HCI avec le scan/publicité BLE étendus.
// Arduino-ESP32 fournit parfois 70 octets dans sdkconfig.h, alors que NimBLE
// 2.5.2 exige 257 octets lorsque CONFIG_BT_NIMBLE_EXT_ADV est activé.
#if defined(CONFIG_BT_NIMBLE_EXT_ADV) && CONFIG_BT_NIMBLE_EXT_ADV
#include <sdkconfig.h>
#if defined(CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE) && CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE != 257
#undef CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE
#define CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE 257
#endif
#ifndef CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE
#define CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE 257
#endif
#endif
