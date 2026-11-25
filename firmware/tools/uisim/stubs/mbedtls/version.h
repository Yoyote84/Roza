#pragma once

// The UI simulator uses the local Mbed TLS API shims under stubs/.
// Those shims expose the legacy Mbed TLS 2.x *_ret function names.
#define MBEDTLS_VERSION_NUMBER 0x021C0000
#define MBEDTLS_VERSION_STRING "2.28.0"
#define MBEDTLS_VERSION_STRING_FULL "mbed TLS 2.28.0"
