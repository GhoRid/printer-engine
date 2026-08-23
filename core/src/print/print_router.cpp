#include "print_router.h"

#include "access_pass_print.h"
#include "receipt_print.h"

#include <charconv>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {

struct JsonValue {
    bool isString = false;
    std::string stringValue;
    int intValue = 0;
};

using JsonObject = std::unordered_map<std::string, JsonValue>;

void appendUtf8(std::string& output, unsigned int codePoint)
{
    if (codePoint <= 0x7F) {
        output.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else {
        output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

class FlatJsonParser
{
public:
    explicit FlatJsonParser(std::string_view input) : input_(input) {}

    bool parse(JsonObject& output)
    {
        skipWhitespace();

        if (!consume('{')) return false;
        skipWhitespace();

        if (consume('}')) return finish();

        while (true) {
            std::string key;
            JsonValue value;

            if (!parseString(key)) return false;
            skipWhitespace();
            if (!consume(':')) return false;
            skipWhitespace();

            if (peek() == '"') {
                value.isString = true;
                if (!parseString(value.stringValue)) return false;
            }
            else if (!parseInt(value.intValue)) {
                return false;
            }

            if (!output.emplace(std::move(key), std::move(value)).second) {
                return false;
            }

            skipWhitespace();
            if (consume('}')) return finish();
            if (!consume(',')) return false;
            skipWhitespace();
        }
    }

private:
    std::string_view input_;
    std::size_t position_ = 0;

    char peek() const
    {
        return position_ < input_.size() ? input_[position_] : '\0';
    }

    bool consume(char expected)
    {
        if (peek() != expected) return false;
        ++position_;
        return true;
    }

    void skipWhitespace()
    {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_]))) {
            ++position_;
        }
    }

    bool finish()
    {
        skipWhitespace();
        return position_ == input_.size();
    }

    bool parseString(std::string& output)
    {
        if (!consume('"')) return false;

        while (position_ < input_.size()) {
            const unsigned char ch = static_cast<unsigned char>(input_[position_++]);

            if (ch == '"') return true;
            if (ch < 0x20) return false;

            if (ch != '\\') {
                output.push_back(static_cast<char>(ch));
                continue;
            }

            if (position_ >= input_.size()) return false;
            const char escaped = input_[position_++];

            switch (escaped) {
                case '"': output.push_back('"'); break;
                case '\\': output.push_back('\\'); break;
                case '/': output.push_back('/'); break;
                case 'b': output.push_back('\b'); break;
                case 'f': output.push_back('\f'); break;
                case 'n': output.push_back('\n'); break;
                case 'r': output.push_back('\r'); break;
                case 't': output.push_back('\t'); break;
                case 'u': {
                    unsigned int codePoint = 0;

                    for (int i = 0; i < 4; ++i) {
                        if (position_ >= input_.size()) return false;
                        const char hex = input_[position_++];
                        codePoint <<= 4;

                        if (hex >= '0' && hex <= '9') codePoint |= hex - '0';
                        else if (hex >= 'a' && hex <= 'f') codePoint |= hex - 'a' + 10;
                        else if (hex >= 'A' && hex <= 'F') codePoint |= hex - 'A' + 10;
                        else return false;
                    }

                    if (codePoint >= 0xD800 && codePoint <= 0xDFFF) return false;
                    appendUtf8(output, codePoint);
                    break;
                }
                default: return false;
            }
        }

        return false;
    }

    bool parseInt(int& output)
    {
        const std::size_t start = position_;

        if (peek() == '-') ++position_;
        const std::size_t digits = position_;

        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_]))) {
            ++position_;
        }

        if (digits == position_) return false;

        const char* begin = input_.data() + start;
        const char* end = input_.data() + position_;
        const auto result = std::from_chars(begin, end, output);
        return result.ec == std::errc{} && result.ptr == end;
    }
};

const std::string* getString(const JsonObject& object, const char* key)
{
    const auto found = object.find(key);
    return found != object.end() && found->second.isString
        ? &found->second.stringValue
        : nullptr;
}

const int* getInt(const JsonObject& object, const char* key)
{
    const auto found = object.find(key);
    return found != object.end() && !found->second.isString
        ? &found->second.intValue
        : nullptr;
}

PrintRouteResult invalidBody()
{
    return {400, R"({"error":"invalid_print_data"})"};
}

PrintRouteResult printResult(bool printed)
{
    return printed
        ? PrintRouteResult{200, R"({"ok":true})"}
        : PrintRouteResult{503, R"({"error":"print_failed"})"};
}

bool isSafeText(const std::string& value, std::size_t maxSize, bool optional = false)
{
    if (value.empty()) return optional;
    if (value.size() > maxSize) return false;

    for (const unsigned char ch : value) {
        if (ch < 0x20 || ch == 0x7F) return false;
    }

    return true;
}

} // namespace

PrintRouteResult routePrintRequest(
    const std::string& path,
    const std::string& body,
    PE_Printer* printer
)
{
    if (path != "/print/receipt" && path != "/print/access-pass") {
        return {404, R"({"error":"not_found"})"};
    }

    JsonObject object;

    if (!FlatJsonParser(body).parse(object)) {
        return invalidBody();
    }

    if (path == "/print/receipt") {
        const std::string* name = getString(object, "name");
        const std::string* offeringType = getString(object, "offeringType");
        const int* amount = getInt(object, "amount");

        if (!name || !isSafeText(*name, 256) ||
            (offeringType && !isSafeText(*offeringType, 256, true)) ||
            !amount || *amount <= 0) {
            return invalidBody();
        }

        return printResult(printReceipt(
            printer,
            {*name, offeringType ? *offeringType : "", *amount}
        ));
    }

    if (path == "/print/access-pass") {
        const std::string* name = getString(object, "name");
        const std::string* department = getString(object, "department");
        const std::string* qrValue = getString(object, "qrValue");

        if (!name || !isSafeText(*name, 256) ||
            (department && !isSafeText(*department, 256, true)) ||
            !qrValue || qrValue->empty() || qrValue->size() > 1024) {
            return invalidBody();
        }

        return printResult(printAccessPass(
            printer,
            {*name, department ? *department : "", *qrValue}
        ));
    }

    return {404, R"({"error":"not_found"})"};
}
