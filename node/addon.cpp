#include <napi.h>

#include "image_loader.h"
#include "printer_engine.h"
#include "serial_port.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace {

PE_Printer* printer = nullptr;

struct FormStep {
    PE_CommandType type;
    std::string text;
    std::string secondaryText;
    int value = 0;
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> data;
};

std::unordered_map<std::string, std::vector<FormStep>> forms;

void cleanup()
{
    pe_destroy(printer);
    printer = nullptr;
    forms.clear();
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

bool commandType(const std::string& name, PE_CommandType& type)
{
    if (name == "text") type = PE_COMMAND_TEXT;
    else if (name == "left") type = PE_COMMAND_ALIGN_LEFT;
    else if (name == "center") type = PE_COMMAND_ALIGN_CENTER;
    else if (name == "right") type = PE_COMMAND_ALIGN_RIGHT;
    else if (name == "feed") type = PE_COMMAND_FEED;
    else if (name == "qr") type = PE_COMMAND_QR;
    else if (name == "image") type = PE_COMMAND_IMAGE;
    else if (name == "cut") type = PE_COMMAND_CUT;
    else if (name == "columns") type = PE_COMMAND_COLUMNS;
    else return false;
    return true;
}

Napi::Value setForms(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "setForms(forms) requires an object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::unordered_map<std::string, std::vector<FormStep>> parsed;
    const Napi::Object input = info[0].As<Napi::Object>();
    const Napi::Array names = input.GetPropertyNames();

    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string name = names.Get(i).As<Napi::String>().Utf8Value();
        const Napi::Value formValue = input.Get(name);
        if (!formValue.IsArray()) {
            Napi::TypeError::New(env, "each form must be an array")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        const Napi::Array steps = formValue.As<Napi::Array>();
        auto& form = parsed[name];
        form.reserve(steps.Length());

        for (uint32_t j = 0; j < steps.Length(); ++j) {
            if (!steps.Get(j).IsObject()) {
                Napi::TypeError::New(env, "each form step must be an object")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }

            const Napi::Object step = steps.Get(j).As<Napi::Object>();
            const std::string typeName = stringOption(step, "type");
            FormStep parsedStep{};
            if (!commandType(typeName, parsedStep.type)) {
                Napi::TypeError::New(env, "unknown form step: " + typeName)
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }

            if (parsedStep.type == PE_COMMAND_TEXT || parsedStep.type == PE_COMMAND_QR) {
                parsedStep.text = stringOption(step, "value");
                if (parsedStep.text.empty()) {
                    Napi::TypeError::New(env, "text and qr steps require value")
                        .ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (parsedStep.type == PE_COMMAND_COLUMNS) {
                parsedStep.text = stringOption(step, "left");
                parsedStep.secondaryText = stringOption(step, "right");
                if (parsedStep.text.empty() || parsedStep.secondaryText.empty()) {
                    Napi::TypeError::New(env, "columns steps require left and right")
                        .ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (parsedStep.type == PE_COMMAND_IMAGE) {
                const std::string path = stringOption(step, "value");
                const int width = intOption(step, "width", 0);
                const int threshold = intOption(step, "threshold", 160);
                if (path.empty() || width < 0 || width > 4096 ||
                    threshold < 0 || threshold > 255) {
                    Napi::TypeError::New(
                        env,
                        "image requires value; width must be 0..4096 and threshold 0..255"
                    ).ThrowAsJavaScriptException();
                    return env.Undefined();
                }

                LoadedImage image;
                std::string error;
                if (!loadImageAsMonochrome(
                        path, width, threshold, image, error
                    )) {
                    Napi::TypeError::New(env, error).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                parsedStep.width = image.width;
                parsedStep.height = image.height;
                parsedStep.data = std::move(image.data);
            }
            else if (parsedStep.type == PE_COMMAND_FEED) {
                parsedStep.value = intOption(step, "lines", 1);
                if (parsedStep.value < 1) {
                    Napi::TypeError::New(env, "feed lines must be positive")
                        .ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }

            form.push_back(std::move(parsedStep));
        }
    }

    forms = std::move(parsed);
    return Napi::Boolean::New(env, true);
}

bool render(
    const std::string& input,
    const std::unordered_map<std::string, std::string>& values,
    std::string& output
)
{
    std::size_t position = 0;
    while (position < input.size()) {
        const std::size_t start = input.find("{{", position);
        if (start == std::string::npos) {
            output.append(input, position);
            return true;
        }
        output.append(input, position, start - position);
        const std::size_t end = input.find("}}", start + 2);
        if (end == std::string::npos) return false;
        const auto value = values.find(input.substr(start + 2, end - start - 2));
        if (value == values.end()) return false;
        output += value->second;
        position = end + 2;
    }
    return true;
}

Napi::Value printForm(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 2 || !info[0].IsString() || !info[1].IsObject()) {
        Napi::TypeError::New(env, "print(form, values) requires a form and values")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string name = info[0].As<Napi::String>().Utf8Value();
    const auto form = forms.find(name);
    if (form == forms.end()) {
        Napi::TypeError::New(env, "unknown form: " + name).ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::unordered_map<std::string, std::string> values;
    const Napi::Object input = info[1].As<Napi::Object>();
    const Napi::Array names = input.GetPropertyNames();
    for (uint32_t i = 0; i < names.Length(); ++i) {
        const std::string key = names.Get(i).As<Napi::String>().Utf8Value();
        values[key] = input.Get(key).ToString().Utf8Value();
    }

    std::vector<std::string> rendered(form->second.size());
    std::vector<std::string> renderedSecondary(form->second.size());
    std::vector<PE_PrintCommand> commands;
    commands.reserve(form->second.size());
    for (std::size_t i = 0; i < form->second.size(); ++i) {
        const FormStep& step = form->second[i];
        if (!step.text.empty() && !render(step.text, values, rendered[i])) {
            Napi::TypeError::New(env, "missing or invalid form value")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (!step.secondaryText.empty() &&
            !render(step.secondaryText, values, renderedSecondary[i])) {
            Napi::TypeError::New(env, "missing or invalid form value")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        commands.push_back({
            step.type,
            step.text.empty() ? nullptr : rendered[i].c_str(),
            step.value,
            step.data.empty() ? nullptr : step.data.data(),
            step.data.size(),
            step.width,
            step.height,
            step.secondaryText.empty() ? nullptr : renderedSecondary[i].c_str()
        });
    }

    const PE_Result result = pe_print_commands(printer, commands.data(), commands.size());
    if (result != PE_OK) {
        throwResult(env, result);
        return env.Undefined();
    }
    return Napi::Boolean::New(env, true);
}

Napi::Value printJson(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2 || !info[0].IsString() ||
        (!info[1].IsObject() && !info[1].IsString())) {
        Napi::TypeError::New(env, "printJson(form, data) requires a form and JSON object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string form = info[0].As<Napi::String>().Utf8Value();
    std::string json;

    if (info[1].IsString()) {
        json = info[1].As<Napi::String>().Utf8Value();
    }
    else {
        const Napi::Function stringify = env.Global()
            .Get("JSON").As<Napi::Object>()
            .Get("stringify").As<Napi::Function>();
        const Napi::Value value = stringify.Call({info[1]});

        if (env.IsExceptionPending()) return env.Undefined();
        json = value.As<Napi::String>().Utf8Value();
    }

    const PE_Result result = pe_print_json(printer, form.c_str(), json.c_str());
    if (result != PE_OK) {
        throwResult(env, result);
        return env.Undefined();
    }

    return Napi::Boolean::New(env, true);
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
    const int paddingLeftDots = intOption(input, "paddingLeftDots", 0);
    const int paddingRightDots = intOption(input, "paddingRightDots", 0);
    const int textWidthColumns = intOption(input, "textWidthColumns", 0);

    if (textWidthColumns > 0 &&
        (paddingLeftDots > 0 || paddingRightDots > 0)) {
        Napi::TypeError::New(
            env,
            "textWidthColumns cannot be used with paddingLeftDots or paddingRightDots"
        ).ThrowAsJavaScriptException();
        return env.Undefined();
    }

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
        paddingLeftDots,
        paddingRightDots,
        intOption(input, "asciiCharWidthDots", 12),
        textWidthColumns,
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
    std::string text;
    if (info.Length() > 0 && !info[0].IsUndefined()) {
        if (!info[0].IsString()) {
            Napi::TypeError::New(info.Env(), "text must be a string")
                .ThrowAsJavaScriptException();
            return info.Env().Undefined();
        }
        text = info[0].As<Napi::String>().Utf8Value();
    }

    const PE_Result result = text.empty()
        ? pe_print_test(printer)
        : pe_print_test_text(printer, text.c_str());
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

Napi::Value getComPorts(const Napi::CallbackInfo& info)
{
    const std::vector<std::string> ports = SerialPort::listPorts();
    Napi::Array result = Napi::Array::New(info.Env(), ports.size());
    for (std::size_t i = 0; i < ports.size(); ++i) result.Set(i, ports[i]);
    return result;
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
    exports.Set("printJson", Napi::Function::New(env, printJson));
    exports.Set("setForms", Napi::Function::New(env, setForms));
    exports.Set("print", Napi::Function::New(env, printForm));
    exports.Set("getPrinterType", Napi::Function::New(env, getPrinterType));
    exports.Set("getComPorts", Napi::Function::New(env, getComPorts));
    exports.Set("shutdown", Napi::Function::New(env, shutdown));
    return exports;
}

} // namespace

NODE_API_MODULE(printer_engine_node, init)
