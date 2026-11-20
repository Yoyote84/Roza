package fr.roza.hawkinsai

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class HawkinsProtocolTest {
    @Test
    fun truncationKeepsUtf8CodePointsWhole() {
        val text = "abc éè漢字 😀 tail"
        val limited = HawkinsProtocol.fitUtf8(text, 8)
        assertEquals("abc éè", limited)
        assertTrue(limited.toByteArray(Charsets.UTF_8).size <= 8)
    }

    @Test
    fun frameCodecUsesVersionedPrefixAndUtf8() {
        val bytes = HawkinsProtocol.encodeNarration(
            9L, "La radio gresille.", listOf("Repondre", "Ecouter", "Se cacher")
        )
        val frame = String(bytes, Charsets.UTF_8)
        assertTrue(frame.startsWith("HKA/1 "))
        assertTrue(frame.endsWith("\n"))
        assertTrue(frame.contains("\"id\":9"))
        assertTrue(frame.contains("\"choices\""))
    }

    @Test
    fun parserPreservesUtf8Text() {
        val frame = """HKA/1 {"type":"narrate","id":5,"context":{"memory":"Lia a aidé l'été dernier"}}"""
        assertEquals("Lia a aidé l'été dernier",
            HawkinsProtocol.parseFrame(frame)?.getJSONObject("context")?.getString("memory"))
    }

    @Test
    fun parserIgnoresLegacySerialOutput() {
        assertEquals(null, HawkinsProtocol.parseFrame("[boot-diag] ready"))
    }
}


    @Test
    fun cardputerAcknowledgementIsRecognized() {
        val frame = """HKA/1 {"type":"ack","id":1,"status":"narration_accepted"}"""
        val message = HawkinsProtocol.parseFrame(frame)
        assertEquals("ack", message?.getString("type"))
        assertEquals(1L, message?.getLong("id"))
        assertEquals("narration_accepted", message?.getString("status"))
    }

    @Test
    fun cardputerNegativeAcknowledgementIsRecognized() {
        val frame = """HKA/1 {"type":"nack","id":1,"status":"invalid_frame"}"""
        val message = HawkinsProtocol.parseFrame(frame)
        assertEquals("nack", message?.getString("type"))
        assertEquals("invalid_frame", message?.getString("status"))
    }
