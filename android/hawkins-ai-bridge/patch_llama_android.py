#!/usr/bin/env python3
"""Patch the pinned llama.cpp Android sample to expose real model-load diagnostics."""

from pathlib import Path

repo_root = Path(__file__).resolve().parents[2]
submodule = repo_root / "android" / "llama.cpp"
cpp = submodule / "examples/llama.android/lib/src/main/cpp/ai_chat.cpp"
iface = submodule / "examples/llama.android/lib/src/main/java/com/arm/aichat/InferenceEngine.kt"
impl = submodule / "examples/llama.android/lib/src/main/java/com/arm/aichat/internal/InferenceEngineImpl.kt"

for path in (cpp, iface, impl):
    if not path.is_file():
        raise SystemExit(
            f"llama.cpp Android source not found: {path}\n"
            "Initialize submodules with: git submodule update --init --recursive"
        )

cpp_text = cpp.read_text(encoding="utf-8")
iface_text = iface.read_text(encoding="utf-8")
impl_text = impl.read_text(encoding="utf-8")

if "Java_com_arm_aichat_internal_InferenceEngineImpl_getLastModelLoadDiagnostic" in cpp_text:
    print("llama.cpp native GGUF diagnostics already applied.")
    raise SystemExit(0)

def replace_once(content: str, old: str, new: str, label: str) -> str:
    count = content.count(old)
    if count != 1:
        raise SystemExit(f"Cannot safely patch {label}: expected 1 match, found {count}")
    return content.replace(old, new, 1)

cpp_text = replace_once(
    cpp_text,
    "#include <string>\n",
    "#include <string>\n#include <mutex>\n",
    "C++ mutex include",
)
cpp_text = replace_once(
    cpp_text,
    "static common_sampler                   * g_sampler;\n",
    """static common_sampler                   * g_sampler;

static std::mutex g_model_load_log_mutex;
static std::string g_model_load_log;
static bool g_capture_model_load_log = false;

static void aichat_model_load_log_callback(
        enum ggml_log_level level,
        const char * text,
        void * user_data) {
    if (text != nullptr) {
        std::lock_guard<std::mutex> lock(g_model_load_log_mutex);
        if (g_capture_model_load_log) {
            g_model_load_log.append(text);
            constexpr size_t MAX_MODEL_LOAD_LOG = 16000;
            if (g_model_load_log.size() > MAX_MODEL_LOAD_LOG) {
                g_model_load_log.erase(
                    0, g_model_load_log.size() - MAX_MODEL_LOAD_LOG);
            }
        }
        aichat_android_log_callback(level, text, user_data);
    }
}
""",
    "native model log capture",
)
cpp_text = replace_once(
    cpp_text,
    "llama_log_set(aichat_android_log_callback, nullptr);",
    "llama_log_set(aichat_model_load_log_callback, nullptr);",
    "native log callback registration",
)
cpp_text = replace_once(
    cpp_text,
    """    auto *model = llama_model_load_from_file(model_path, model_params);
    env->ReleaseStringUTFChars(jmodel_path, model_path);
""",
    """    {
        std::lock_guard<std::mutex> lock(g_model_load_log_mutex);
        g_model_load_log.clear();
        g_capture_model_load_log = true;
    }
    auto *model = llama_model_load_from_file(model_path, model_params);
    {
        std::lock_guard<std::mutex> lock(g_model_load_log_mutex);
        g_capture_model_load_log = false;
    }
    env->ReleaseStringUTFChars(jmodel_path, model_path);
""",
    "model load diagnostic capture",
)
cpp_text = replace_once(
    cpp_text,
    """    g_model = model;
    return 0;
}

static llama_context *init_context""",
    """    g_model = model;
    return 0;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_arm_aichat_internal_InferenceEngineImpl_getLastModelLoadDiagnostic(
        JNIEnv *env, jobject /*unused*/) {
    std::lock_guard<std::mutex> lock(g_model_load_log_mutex);
    return env->NewStringUTF(g_model_load_log.c_str());
}

static llama_context *init_context""",
    "JNI diagnostic accessor",
)

iface_text = replace_once(
    iface_text,
    "    suspend fun loadModel(pathToModel: String)\n",
    "    suspend fun loadModel(pathToModel: String)\n\n    /** Returns native llama.cpp log output from the most recent model-load attempt. */\n    fun lastModelLoadDiagnostic(): String\n",
    "inference interface diagnostic method",
)
impl_text = replace_once(
    impl_text,
    "    private external fun load(modelPath: String): Int\n",
    "    private external fun load(modelPath: String): Int\n\n    @FastNative\n    private external fun getLastModelLoadDiagnostic(): String\n\n    override fun lastModelLoadDiagnostic(): String = getLastModelLoadDiagnostic()\n",
    "inference JNI diagnostic method",
)

cpp.write_text(cpp_text, encoding="utf-8")
iface.write_text(iface_text, encoding="utf-8")
impl.write_text(impl_text, encoding="utf-8")
print("Applied GGUF metadata and native llama.cpp load diagnostics.")
