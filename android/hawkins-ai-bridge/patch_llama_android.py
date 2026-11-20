#!/usr/bin/env python3
"""Patch the pinned llama.cpp Android sample for reliable CPU backend loading and diagnostics."""

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


def replace_once(content: str, old: str, new: str, label: str) -> str:
    count = content.count(old)
    if count != 1:
        raise SystemExit(f"Cannot safely patch {label}: expected 1 match, found {count}")
    return content.replace(old, new, 1)


# Native diagnostic capture is patched once. Keep the backend-init correction
# independent so later improvements to this script also work on an already-patched checkout.
if "Java_com_arm_aichat_internal_InferenceEngineImpl_getLastModelLoadDiagnostic" not in cpp_text:
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
static std::string g_backend_init_diagnostic;

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
    const std::string diagnostic =
        g_backend_init_diagnostic + "\\n" + g_model_load_log;
    return env->NewStringUTF(diagnostic.c_str());
}

static llama_context *init_context""",
        "JNI diagnostic accessor",
    )

if "lastModelLoadDiagnostic(): String" not in iface_text:
    iface_text = replace_once(
        iface_text,
        "    suspend fun loadModel(pathToModel: String)\n",
        "    suspend fun loadModel(pathToModel: String)\n\n"
        "    /** Returns native llama.cpp log output from the most recent model-load attempt. */\n"
        "    fun lastModelLoadDiagnostic(): String\n",
        "inference interface diagnostic method",
    )

if "getLastModelLoadDiagnostic(): String" not in impl_text:
    impl_text = replace_once(
        impl_text,
        "    private external fun load(modelPath: String): Int\n",
        "    private external fun load(modelPath: String): Int\n\n"
        "    @FastNative\n"
        "    private external fun getLastModelLoadDiagnostic(): String\n\n"
        "    override fun lastModelLoadDiagnostic(): String = getLastModelLoadDiagnostic()\n",
        "inference JNI diagnostic method",
    )

# Ensure the diagnostic global also exists when this script is rerun against
# a checkout that already contains the earlier diagnostics patch.
if "static std::string g_backend_init_diagnostic;" not in cpp_text:
    cpp_text = replace_once(
        cpp_text,
        "static common_sampler                   * g_sampler;\n",
        "static common_sampler                   * g_sampler;\n"
        "static std::string g_backend_init_diagnostic;\n",
        "backend initialization diagnostic storage",
    )

# Force a baseline CPU backend load if directory discovery did not register one.
if "CPU backend registration check:" not in cpp_text:
    cpp_text = replace_once(
        cpp_text,
        """    ggml_backend_load_all_from_path(path_to_backend);
    env->ReleaseStringUTFChars(nativeLibDir, path_to_backend);

    // Initialize backends
""",
        """    ggml_backend_load_all_from_path(path_to_backend);

    // Android arm64 packages a safe ARMv8.0 CPU plugin. If dynamic discovery
    // did not register CPU, try that exact file explicitly and preserve the reason.
    const std::string baseline_cpu_backend =
        std::string(path_to_backend) + "/libggml-cpu-android_armv8.0_1.so";
    if (ggml_backend_reg_by_name("CPU") == nullptr &&
        access(baseline_cpu_backend.c_str(), R_OK) == 0) {
        LOGw("CPU backend registration check: retrying explicit plugin load: %s",
             baseline_cpu_backend.c_str());
        ggml_backend_load(baseline_cpu_backend.c_str());
    }

    if (ggml_backend_reg_by_name("CPU") == nullptr) {
        g_backend_init_diagnostic =
            "CPU backend NOT registered. nativeLibraryDir=" +
            std::string(path_to_backend) +
            "; baseline_plugin_exists=" +
            (access(baseline_cpu_backend.c_str(), R_OK) == 0 ? "yes" : "no") +
            ". Android must extract the JNI libraries before llama.cpp can load them.";
        LOGe("%s", g_backend_init_diagnostic.c_str());
    } else {
        g_backend_init_diagnostic =
            "CPU backend registered from nativeLibraryDir=" +
            std::string(path_to_backend) + ".";
        LOGi("%s", g_backend_init_diagnostic.c_str());
    }

    env->ReleaseStringUTFChars(nativeLibDir, path_to_backend);

    // Initialize backends
""",
        "CPU backend registration check",
    )

# Respect the requested generation limit exactly. current_position has already
# been advanced by user_prompt_size, so adding that size again doubled the
# output allowance and made short narration requests unnecessarily slow.
if "stop_generation_position = current_position + user_prompt_size + n_predict;" in cpp_text:
    cpp_text = replace_once(
        cpp_text,
        """    current_position += user_prompt_size;
    stop_generation_position = current_position + user_prompt_size + n_predict;
""",
        """    current_position += user_prompt_size;
    stop_generation_position = current_position + n_predict;
""",
        "exact generation token limit",
    )
elif "stop_generation_position = current_position + n_predict;" not in cpp_text:
    raise SystemExit("Cannot verify llama.cpp generation token limit patch.")

cpp.write_text(cpp_text, encoding="utf-8")
iface.write_text(iface_text, encoding="utf-8")
impl.write_text(impl_text, encoding="utf-8")
print("Applied GGUF diagnostics and Android CPU backend loading safeguards.")
