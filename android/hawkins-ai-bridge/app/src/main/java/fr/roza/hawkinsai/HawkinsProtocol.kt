package fr.roza.hawkinsai

import org.json.JSONArray
import org.json.JSONObject

internal object HawkinsProtocol {
    const val PREFIX = "HKA/1 "
    const val MAX_TEXT_BYTES = 210
    const val MAX_CHOICE_BYTES = 34
    const val MAX_CHOICES = 3

    fun parseFrame(line: String): JSONObject? {
        if (!line.startsWith(PREFIX) || line.length > 1792) return null
        return runCatching { JSONObject(line.substring(PREFIX.length)) }.getOrNull()
    }

    fun encodeHello(): ByteArray =
        "$PREFIX{\"type\":\"hello\",\"version\":1}\n".toByteArray(Charsets.UTF_8)

    fun encodeError(id: Long, message: String): ByteArray {
        val json = JSONObject()
            .put("type", "error")
            .put("id", id)
            .put("message", fitUtf8(message, 150))
        return "$PREFIX$json\n".toByteArray(Charsets.UTF_8)
    }

    fun encodeNarration(id: Long, text: String, choices: List<String>): ByteArray {
        val jsonChoices = JSONArray()
        choices.take(MAX_CHOICES).forEach {
            jsonChoices.put(fitUtf8(it, MAX_CHOICE_BYTES))
        }
        val json = JSONObject()
            .put("type", "narration")
            .put("id", id)
            .put("text", fitUtf8(text, MAX_TEXT_BYTES))
            .put("choices", jsonChoices)
        return "$PREFIX$json\n".toByteArray(Charsets.UTF_8)
    }

    fun fitUtf8(value: String, maxBytes: Int): String {
        if (maxBytes <= 0) return ""
        val result = StringBuilder()
        var index = 0
        var used = 0
        while (index < value.length) {
            val codePoint = value.codePointAt(index)
            val bytes = when {
                codePoint <= 0x7F -> 1
                codePoint <= 0x7FF -> 2
                codePoint <= 0xFFFF -> 3
                else -> 4
            }
            if (used + bytes > maxBytes) break
            result.appendCodePoint(codePoint)
            used += bytes
            index += Character.charCount(codePoint)
        }
        return result.toString().trim()
    }
}
