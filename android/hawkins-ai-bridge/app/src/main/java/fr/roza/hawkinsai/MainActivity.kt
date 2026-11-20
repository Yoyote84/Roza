package fr.roza.hawkinsai

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import com.arm.aichat.AiChat
import com.arm.aichat.gguf.GgufMetadataReader
import com.arm.aichat.InferenceEngine
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.util.SerialInputOutputManager
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.io.FileInputStream
import java.io.ByteArrayOutputStream
import java.nio.file.Files
import java.nio.file.StandardCopyOption

class MainActivity : ComponentActivity() {
    companion object {
        private const val ACTION_USB_PERMISSION = "fr.roza.hawkinsai.USB_PERMISSION"
        private const val SERIAL_BAUD = 115200
        private const val MAX_INCOMING_LINE = 4096
        private const val GENERATION_TIMEOUT_MS = 240_000L

        private const val SYSTEM_PROMPT = """
Tu es le maitre du jeu d'une aventure de role-play Hawkins inspiree de l'ambiance Stranger Things.
Tu racontes en francais, avec une atmosphere mysterieuse, humaine, tendue et cinematographique.
Tu ne controles jamais le joueur et tu ne modifies jamais l'etat reel du jeu.
Le JSON fourni par le Cardputer est la seule source de verite pour le personnage, les missions,
les objets, les PV, le stress, l'XP, les secteurs et les souvenirs. Ne pretend jamais qu'une
valeur a change. N'invente aucune position, adresse ou coordonnee GPS. Tu peux decrire des
evenements fictifs dans la narration, mais ils ne deviennent pas des faits de jeu tant que le
Cardputer ne les a pas enregistres.
Si player_action est present, interprete-le comme le choix que le joueur vient de faire.
Reponds uniquement avec un objet JSON valide de cette forme :
{"text":"une narration en 2 ou 3 phrases courtes","choices":["choix 1","choix 2","choix 3"]}
Le champ text doit rester sous 190 octets UTF-8. Fournis de 1 a 3 choix courts, chacun sous
30 octets UTF-8. Les choix doivent etre distincts et jouables au clavier. N'utilise pas de
Markdown, pas de bloc de code, pas de texte avant ou apres le JSON, pas d'accents, pas de
raisonnement interne. Ne propose jamais une modification directe de statistiques ou de sauvegarde.
"""
    }

    private val appScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private val pickModel = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri != null) importModel(uri)
    }

    private lateinit var statusView: TextView
    private lateinit var modelView: TextView
    private lateinit var logView: TextView
    private var modelFile: File? = null
    @Volatile private var modelReady = false
    @Volatile private var engine: InferenceEngine? = null
    @Volatile private var generationJob: Job? = null
    private var heartbeatJob: Job? = null
    private var usbManager: UsbManager? = null
    private var serialPort: UsbSerialPort? = null
    private var usbConnection: android.hardware.usb.UsbDeviceConnection? = null
    @Volatile private var ioManager: SerialInputOutputManager? = null
    private val incomingLine = ByteArrayOutputStream()
    private val lineLock = Any()
    private var receiverRegistered = false

    private val usbPermissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action != ACTION_USB_PERMISSION) return
            val device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice::class.java)
            val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
            if (granted && device != null) openUsbDevice(device)
            else setStatus("Autorisation USB refusee.")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager
        modelFile = File(filesDir, "models/hawkins-model.gguf")
            .takeIf { it.isFile && it.length() > 0L }
        buildUi()
        registerReceiver(
            usbPermissionReceiver,
            IntentFilter(ACTION_USB_PERMISSION),
            Context.RECEIVER_NOT_EXPORTED
        )
        receiverRegistered = true
        refreshModelLabel()
        setStatus("Pret. Importe un modele GGUF, puis branche le Cardputer en USB-C.")
    }

    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(22, 26, 22, 20)
            gravity = Gravity.TOP
            setBackgroundColor(0xFF101319.toInt())
        }
        fun text(value: String, size: Float, color: Int, bold: Boolean = false): TextView =
            TextView(this).apply {
                text = value
                textSize = size
                setTextColor(color)
                if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
                setPadding(0, 5, 0, 9)
            }
        root.addView(text("HAWKINS AI", 26f, 0xFFFF5252.toInt(), true))
        root.addView(text("Narrateur local • USB-C • sans reseau", 14f, 0xFFE0E0E0.toInt()))
        modelView = text("Aucun modele GGUF importe.", 13f, 0xFF80CBC4.toInt())
        root.addView(modelView)
        root.addView(actionButton("1  Choisir un modele GGUF") {
            pickModel.launch(arrayOf("*/*"))
        })
        root.addView(actionButton("2  Charger le modele sur le telephone") { loadModel() })
        root.addView(actionButton("3  Connecter le Cardputer en USB") { discoverUsbDevice() })
        root.addView(actionButton("Deconnecter le Cardputer") { closeUsb("USB deconnecte.") })
        statusView = text("Initialisation...", 14f, 0xFFFFD180.toInt(), true)
        root.addView(statusView)
        root.addView(text(
            "Le modele est copie dans le stockage prive de l'application. Aucun modele n'est " +
                "inclus dans l'APK et aucune permission Internet n'est demandee. Une fois le " +
                "fichier GGUF importe, l'inference fonctionne hors ligne.",
            12f, 0xFFB0BEC5.toInt()
        ))
        logView = text("", 11f, 0xFFCFD8DC.toInt())
        val scroll = ScrollView(this).apply { addView(logView) }
        root.addView(scroll, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f
        ))
        setContentView(root)
    }

    private fun actionButton(label: String, action: () -> Unit): Button =
        Button(this).apply {
            text = label
            isAllCaps = false
            setOnClickListener { action() }
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = 7 }
        }

    private fun refreshModelLabel() {
        val file = modelFile
        modelView.text = if (file?.isFile == true) {
            "Modele : " + file.name + " • " + (file.length() / (1024L * 1024L)) + " Mo"
        } else "Aucun modele GGUF importe."
    }

    private fun importModel(uri: android.net.Uri) {
        appScope.launch {
            setStatus("Copie du modele dans le stockage prive...")
            try {
                val target = withContext(Dispatchers.IO) {
                    val directory = File(filesDir, "models").apply { mkdirs() }
                    val finalFile = File(directory, "hawkins-model.gguf")
                    val temporary = File(directory, "hawkins-model.gguf.part")
                    val expectedSize = contentResolver.openAssetFileDescriptor(uri, "r")?.use {
                        it.length
                    } ?: -1L
                    if (expectedSize > 0L &&
                        filesDir.usableSpace < expectedSize + 256L * 1024L * 1024L) {
                        error("Espace libre insuffisant pour copier le modele.")
                    }
                    contentResolver.openInputStream(uri)?.use { input ->
                        FileOutputStream(temporary).use { output ->
                            val buffer = ByteArray(1024 * 1024)
                            while (true) {
                                val count = input.read(buffer)
                                if (count < 0) break
                                output.write(buffer, 0, count)
                            }
                            output.fd.sync()
                        }
                    } ?: error("Impossible de lire le fichier selectionne.")
                    if (temporary.length() < 1024L * 1024L) {
                        temporary.delete()
                        error("Le fichier semble trop petit pour un modele GGUF.")
                    }
                    FileInputStream(temporary).use { input ->
                        val magic = ByteArray(4)
                        check(input.read(magic) == 4 &&
                            magic.contentEquals("GGUF".toByteArray(Charsets.US_ASCII))) {
                            "Le fichier choisi n'est pas au format GGUF."
                        }
                    }
                    Files.move(
                        temporary.toPath(),
                        finalFile.toPath(),
                        StandardCopyOption.REPLACE_EXISTING
                    )
                    finalFile
                }
                modelFile = target
                modelReady = false
                refreshModelLabel()
                setStatus("Modele importe. Appuie sur Charger.")
                appendLog("Fichier local pret : " + target.name)
            } catch (error: Exception) {
                setStatus("Import impossible : " + (error.message ?: "erreur inconnue"))
            }
        }
    }

    private fun loadModel() {
        val file = modelFile
        if (file == null || !file.isFile) {
            setStatus("Choisis d'abord un fichier GGUF.")
            return
        }
        appScope.launch {
            modelReady = false
            setStatus("Chargement du moteur local et du modele...")
            try {
                val localEngine = withContext(Dispatchers.IO) {
                    AiChat.getInferenceEngine(applicationContext)
                }
                engine = localEngine
                val preferences = getSharedPreferences("hawkins_ai", Context.MODE_PRIVATE)
                val signature = file.absolutePath + ":" + file.length() + ":" + file.lastModified()
                val initialState = localEngine.state.value
                if (initialState is InferenceEngine.State.ModelReady &&
                    preferences.getString("loaded_model_signature", null) == signature) {
                    modelReady = true
                    setStatus("Le modele local est deja charge.")
                    return@launch
                }
                if (initialState is InferenceEngine.State.ModelReady ||
                    initialState is InferenceEngine.State.Error) {
                    withContext(Dispatchers.IO) { localEngine.cleanUp() }
                }
                withTimeout(180_000L) {
                    localEngine.state.first {
                        it is InferenceEngine.State.Initialized ||
                            it is InferenceEngine.State.Error
                    }
                }
                val metadata = withContext(Dispatchers.IO) {
                    FileInputStream(file).use {
                        GgufMetadataReader.create().readStructuredMetadata(it)
                    }
                }
                val architecture = metadata.architecture?.architecture?.takeIf { it.isNotBlank() }
                    ?: "inconnue"
                appendLog(
                    "GGUF valide : architecture=$architecture, version=${metadata.version.label}, " +
                        "tenseurs=${metadata.tensorCount}, métadonnées=${metadata.kvCount}."
                )
                setStatus("GGUF reconnu ($architecture). Chargement du modèle...")
                val current = localEngine.state.value
                if (current is InferenceEngine.State.Error) throw current.exception
                withContext(Dispatchers.IO) {
                    try {
                        localEngine.loadModel(file.absolutePath)
                    } catch (loadError: Exception) {
                        val nativeLog = runCatching {
                            localEngine.lastModelLoadDiagnostic().trim()
                        }.getOrDefault("")
                        val message = buildString {
                            append(
                                "Échec du chargement natif GGUF. Le code actuel de la bibliothèque " +
                                    "renvoie une exception générique, qui ne prouve pas à elle seule " +
                                    "une architecture incompatible."
                            )
                            if (nativeLog.isNotBlank()) {
                                append("\n\nDiagnostic llama.cpp :\n")
                                append(nativeLog.takeLast(4500))
                            } else {
                                append("\nAucun journal natif n'a été remonté.")
                            }
                        }
                        throw java.io.IOException(message, loadError)
                    }
                    localEngine.setSystemPrompt(SYSTEM_PROMPT)
                }
                preferences.edit().putString("loaded_model_signature", signature).apply()
                modelReady = true
                setStatus("Modele charge. Inference locale prete.")
                appendLog("Moteur llama.cpp : modele charge en memoire.")
            } catch (error: Exception) {
                modelReady = false
                val detail = error.message ?: error.javaClass.simpleName
                setStatus("Échec du chargement : " +
                    detail.lineSequence().firstOrNull().orEmpty().take(150))
                appendLog(detail.takeLast(5000))
                appendLog(error.stackTraceToString().take(1000))
            }
        }
    }

    private fun discoverUsbDevice() {
        val manager = usbManager ?: return
        val drivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager)
        if (drivers.isEmpty()) {
            setStatus("Aucun port serie USB detecte. Verifie le cable OTG USB-C et le Cardputer allume.")
            return
        }
        val driver = drivers.first()
        if (!manager.hasPermission(driver.device)) {
            val permission = PendingIntent.getBroadcast(
                this, 0,
                Intent(ACTION_USB_PERMISSION).setPackage(packageName),
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
            )
            manager.requestPermission(driver.device, permission)
            setStatus("Autorisation Android demandee pour le port USB.")
        } else openUsbDevice(driver.device)
    }

    private fun openUsbDevice(device: UsbDevice) {
        closeUsb(null)
        try {
            val manager = usbManager ?: error("USB Host indisponible.")
            val driver = UsbSerialProber.getDefaultProber().probeDevice(device)
                ?: error("Le peripherique n'expose pas de port serie compatible.")
            val port = driver.ports.firstOrNull()
                ?: error("Aucun port serie disponible sur ce peripherique.")
            val connection = manager.openDevice(device)
                ?: error("Android n'a pas pu ouvrir le peripherique USB.")
            serialPort = port
            usbConnection = connection
            port.open(connection)
            port.setParameters(
                SERIAL_BAUD, 8,
                UsbSerialPort.STOPBITS_1,
                UsbSerialPort.PARITY_NONE
            )
            val serial = SerialInputOutputManager(port, object : SerialInputOutputManager.Listener {
                override fun onNewData(data: ByteArray) = consumeSerialData(data)
                override fun onRunError(error: Exception) {
                    runOnUiThread {
                        closeUsb("Liaison USB interrompue : " +
                            (error.message ?: "erreur serie"))
                    }
                }
            })
            serial.setReadTimeout(100)
            ioManager = serial
            serial.start()
            synchronized(lineLock) { incomingLine.reset() }
            setStatus("Cardputer connecte : USB serie 115200, 8N1.")
            appendLog("USB " + device.vendorId.toString(16) + ":" +
                device.productId.toString(16))
            startHeartbeat()
        } catch (error: Exception) {
            setStatus("Connexion USB impossible : " +
                (error.message ?: error.javaClass.simpleName))
            closeUsb(null)
        }
    }

    private fun startHeartbeat() {
        heartbeatJob?.cancel()
        heartbeatJob = appScope.launch(Dispatchers.IO) {
            while (isActive && ioManager != null) {
                writeBytes(HawkinsProtocol.encodeHello())
                delay(5_000L)
            }
        }
    }

    private fun consumeSerialData(data: ByteArray) {
        val completed = ArrayList<String>()
        synchronized(lineLock) {
            data.forEach { byte ->
                val value = byte.toInt() and 0xFF
                if (value == 10) {
                    val rawLine = incomingLine.toByteArray()
                    incomingLine.reset()
                    val lineLength = if (rawLine.lastOrNull()?.toInt() == 13) rawLine.size - 1 else rawLine.size
                    val line = String(rawLine, 0, lineLength, Charsets.UTF_8)
                    if (line.isNotBlank()) completed.add(line)
                } else if (incomingLine.size() < MAX_INCOMING_LINE) {
                    incomingLine.write(value)
                } else incomingLine.reset()
            }
        }
        completed.forEach { line ->
            if (line.startsWith(HawkinsProtocol.PREFIX)) handleProtocolLine(line)
        }
    }

    private fun handleProtocolLine(line: String) {
        val message = HawkinsProtocol.parseFrame(line) ?: return
        when (message.optString("type")) {
            "narrate" -> handleNarrationRequest(message)
        }
    }

    private fun handleNarrationRequest(request: JSONObject) {
        val id = request.optLong("id", 0L)
        if (id <= 0L) return
        val activeEngine = engine
        if (!modelReady || activeEngine == null) {
            writeBytes(HawkinsProtocol.encodeError(
                id, "Charge d'abord le modele GGUF dans Hawkins AI."
            ))
            return
        }
        if (generationJob?.isActive == true) {
            writeBytes(HawkinsProtocol.encodeError(id, "Le narrateur traite deja une scene."))
            return
        }
        val context = request.optJSONObject("context")
        if (context == null) {
            writeBytes(HawkinsProtocol.encodeError(id, "Contexte de partie invalide."))
            return
        }

        runOnUiThread {
            setStatus("Demande #${id} recue. Generation IA locale en cours...")
            appendLog("Demande #${id} recue du Cardputer; contexte ${context.toString().length} caracteres.")
        }
        generationJob = appScope.launch(Dispatchers.IO) {
            var tokenCount = 0
            try {
                val prompt = buildString {
                    append("Etat courant transmis par le Cardputer (JSON faisant autorite) :\n")
                    append(context.toString())
                    append("\nRaconte la prochaine scene et propose des choix. ")
                    append("Si player_action est renseigne, decris les consequences narratives ")
                    append("de cette decision sans modifier les statistiques du jeu.")
                }
                val generated = StringBuilder()
                withTimeout(GENERATION_TIMEOUT_MS) {
                    activeEngine.sendUserPrompt(prompt, 220).collect { token ->
                        if (token.isNotEmpty()) {
                            tokenCount++
                            if (generated.length < 6000) generated.append(token)
                            if (tokenCount == 1 || tokenCount % 16 == 0) {
                                val count = tokenCount
                                runOnUiThread {
                                    setStatus("IA locale en cours : ${count} fragments generes...")
                                }
                            }
                        }
                    }
                }
                val parsed = parseNarratorOutput(generated.toString())
                val narration = HawkinsProtocol.fitUtf8(
                    parsed.first, HawkinsProtocol.MAX_TEXT_BYTES
                )
                val choices = parsed.second.take(HawkinsProtocol.MAX_CHOICES)
                    .map { HawkinsProtocol.fitUtf8(it, HawkinsProtocol.MAX_CHOICE_BYTES) }
                    .filter { it.isNotBlank() }
                if (narration.isBlank()) error("Le modele a renvoye une narration vide.")
                writeBytes(HawkinsProtocol.encodeNarration(id, narration, choices))
                runOnUiThread {
                    setStatus("Scene generee et envoyee au Cardputer.")
                    appendLog("Reponse #${id} envoyee : ${tokenCount} fragments, ${narration.length} caracteres.")
                }
            } catch (timeout: TimeoutCancellationException) {
                writeBytes(HawkinsProtocol.encodeError(
                    id, "Generation trop longue apres 240 secondes. Relancer avec R."
                ))
                runOnUiThread {
                    setStatus("Delai IA depasse. Une erreur a ete renvoyee au Cardputer.")
                    appendLog("Demande #${id} interrompue apres 240 secondes.")
                }
            } catch (cancelled: CancellationException) {
                throw cancelled
            } catch (error: Exception) {
                val detail = error.message ?: error.javaClass.simpleName
                writeBytes(HawkinsProtocol.encodeError(
                    id, "Generation locale impossible : ${detail}"
                ))
                runOnUiThread {
                    setStatus("Erreur d'inference : ${detail}")
                    appendLog("Erreur pour la demande #${id} : ${detail}")
                }
            }
        }
    }

    private fun parseNarratorOutput(raw: String): Pair<String, List<String>> {
        val start = raw.indexOf('{')
        val end = raw.lastIndexOf('}')
        if (start < 0 || end <= start) error("Le modele n'a pas respecte le format JSON.")
        val json = JSONObject(raw.substring(start, end + 1))
        val narration = json.optString("text").trim()
        val options = json.optJSONArray("choices") ?: JSONArray()
        val choices = buildList {
            for (index in 0 until minOf(options.length(), HawkinsProtocol.MAX_CHOICES)) {
                val choice = options.optString(index).trim()
                if (choice.isNotBlank()) add(choice)
            }
        }
        return narration to choices
    }

    private fun writeBytes(bytes: ByteArray) {
        runCatching { ioManager?.writeAsync(bytes) }
            .onFailure {
                runOnUiThread { appendLog("Ecriture USB impossible : " + it.message) }
            }
    }

    private fun closeUsb(message: String?) {
        heartbeatJob?.cancel()
        heartbeatJob = null
        generationJob?.cancel()
        generationJob = null
        runCatching { ioManager?.stop() }
        ioManager = null
        runCatching { serialPort?.close() }
        serialPort = null
        runCatching { usbConnection?.close() }
        usbConnection = null
        synchronized(lineLock) { incomingLine.reset() }
        if (message != null) setStatus(message)
    }

    private fun setStatus(message: String) {
        if (::statusView.isInitialized) statusView.text = message
    }

    private fun appendLog(message: String) {
        if (!::logView.isInitialized) return
        logView.append(message)
        logView.append("\n\n")
    }

    override fun onDestroy() {
        closeUsb(null)
        if (receiverRegistered) {
            runCatching { unregisterReceiver(usbPermissionReceiver) }
            receiverRegistered = false
        }
        appScope.cancel()
        super.onDestroy()
    }
}
