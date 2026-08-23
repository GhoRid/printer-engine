#include <napi.h>

#include "printer_engine.h"

#include <string>

namespace {

PE_Printer* printer = nullptr;

void cleanup()
{
    pe_destroy(printer);
    printer = nullptr;
}

std::string stringOption(
    const Napi::Object& object,
    const char* name,
    const char* fallback = ""
)
{
    const Napi::Value value = object.Get(name);
    return value.IsUndefined() ? fallback : value.As<Napi::String>().Utf8Value();
}

int intOption(const Napi::Object& object, const char* name, int fallback)
{
    const Napi::Value value = object.Get(name);
    return value.IsUndefined() ? fallback : value.As<Napi::Number>().Int32Value();
}

void throwResult(Napi::Env env, PE_Result result)
{
    Napi::Error::New(env, "printer-engine error: " + std::to_string(result))
        .ThrowAsJavaScriptException();
}

Napi::Value initialize(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "initialize(config) requires an object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const Napi::Object input = info[0].As<Napi::Object>();
    const std::string type = stringOption(input, "printerType", "AUTO");
    const std::string port = stringOption(input, "port");
    const PE_PrinterConfig config{
        type.c_str(),
        port.c_str(),
        intOption(input, "baudRate", 115200),
        intOption(input, "dataBits", 8),
        intOption(input, "stopBits", 1),
        intOption(input, "parity", 0),
        intOption(input, "dpi", 203),
        intOption(input, "printWidthDots", 576),
    };

    if (!printer) {
        printer = pe_create();
    }

    if (!printer) {
        Napi::Error::New(env, "failed to allocate printer")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const PE_Result result = pe_initialize(printer, &config);
    if (result != PE_OK) {
        throwResult(env, result);
        return env.Undefined();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value printTest(const Napi::CallbackInfo& info)
{
    const PE_Result result = pe_print_test(printer);
    if (result != PE_OK) {
        throwResult(info.Env(), result);
        return info.Env().Undefined();
    }

    return Napi::Boolean::New(info.Env(), true);
}

Napi::Value getPrinterType(const Napi::CallbackInfo& info)
{
    const char* type = pe_get_printer_type(printer);
    return type ? Napi::String::New(info.Env(), type) : info.Env().Null();
}

Napi::Value shutdown(const Napi::CallbackInfo& info)
{
    pe_destroy(printer);
    printer = nullptr;
    return info.Env().Undefined();
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    env.AddCleanupHook(cleanup);
    exports.Set("initialize", Napi::Function::New(env, initialize));
    exports.Set("printTest", Napi::Function::New(env, printTest));
    exports.Set("getPrinterType", Napi::Function::New(env, getPrinterType));
    exports.Set("shutdown", Napi::Function::New(env, shutdown));
    return exports;
}

} // namespace

NODE_API_MODULE(printer_engine_node, init)
