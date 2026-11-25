#!/usr/bin/env python3
"""Static contract checks for the stage-2 app and upstream refresh."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def source(name: str) -> str:
    return (SRC / name).read_text(encoding="utf-8")


class AppModernizationContracts(unittest.TestCase):
    def test_all_40_menu_apps_are_preserved(self):
        text = source("globals.cpp")
        menu = text.split("const AppDef APPS[] = {", 1)[1].split("};", 1)[0]
        entries = re.findall(r'\{"[^"]+",\s*APP_[A-Z0-9_]+\}', menu)
        self.assertEqual(len(entries), 40)
        self.assertIn("static_assert(APP_COUNT == 40", text)

    def test_http_transport_can_select_http11_and_bound_bodies(self):
        text = source("http_json.h")
        self.assertIn("bool http10;", text)
        self.assertIn("size_t maxBodyBytes;", text)
        self.assertIn("http.useHTTP10(options.http10)", text)
        self.assertIn("response too large", text)
        self.assertIn("HTTPClient::errorToString(code)", text)

    def test_fx_uses_current_frankfurter_v2_schema(self):
        text = source("fx.cpp")
        self.assertIn("api.frankfurter.dev/v2/rates?", text)
        self.assertIn("base=USD&quotes=CNY", text)
        self.assertIn('doc.as<JsonArray>()', text)
        self.assertIn('row["rate"]', text)
        self.assertIn("options.http10 = false", text)

    def test_okx_has_keyless_fallback_and_visible_attribution(self):
        text = source("okx.cpp")
        self.assertIn("api.coinpaprika.com/v1/tickers/usdt-tether", text)
        self.assertIn("api.frankfurter.dev/v2/rate/usd/cny", text)
        self.assertIn("Powered by CoinPaprika", text)
        self.assertIn("SOURCE_COINPAPRIKA", text)
        self.assertNotIn("CoinGecko", text)
        self.assertNotIn("api.coingecko.com", text)

    def test_adsb_and_route_fetches_use_http11_and_limits(self):
        text = source("adsb.cpp")
        self.assertGreaterEqual(text.count("options.http10 = false"), 2)
        self.assertIn("options.maxBodyBytes = 48 * 1024", text)
        self.assertIn("options.maxBodyBytes = 8192", text)

    def test_satellite_errors_survive_json_filtering(self):
        text = source("sats.cpp")
        for field in ('filter["error"]', 'filter["message"]', 'filter["msg"]'):
            self.assertIn(field, text)
        self.assertIn('String("N2YO: ") + providerError', text)
        self.assertIn('no N2YO key in secrets.h', text)

    def test_badapple_rejects_truncated_sd_files_without_writing_them(self):
        text = source("badapple.cpp")
        self.assertIn("requiredBytes", text)
        self.assertIn("vidFile.size()", text)
        self.assertIn("Truncated video data", text)
        self.assertIn("FILE_READ", text)
        self.assertNotIn("FILE_WRITE", text)

    def test_fastled_is_pinned_to_the_reviewed_release(self):
        ini = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("fastled/FastLED@3.10.10", ini)
        self.assertNotIn("fastled/FastLED@3.10.3", ini)


if __name__ == "__main__":
    unittest.main(verbosity=2)
