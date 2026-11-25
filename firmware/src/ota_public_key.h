#pragma once

// Public half of the dedicated ROZA ECDSA P-256 signing key.
// Keep the private key outside Git and store it only as the GitHub Actions
// repository secret ROZA_OTA_SIGNING_KEY_PEM.
static const char ROZA_OTA_PUBLIC_KEY_PEM[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEwJ1pSHxAvO4+aCul4o42QOl70Xzu\n"
    "P6AqLwAhmyvAoGNyXHX8lvrjTVPrk/Dxa3PcxUM/2CZmYLR9uZyA7zyHQA==\n"
    "-----END PUBLIC KEY-----\n";
