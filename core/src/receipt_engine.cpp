#include "receipt_engine.h"
#include "bixolon.h"

#include <algorithm>
#include <string>

namespace pe {

ReceiptEngine::ReceiptEngine(Bixolon& printer, const layout::LayoutConfig& config)
    : printer(printer), layoutConfig(config)
{
}

bool ReceiptEngine::print(const ReceiptData& receipt)
{
    if (!printer.alignCenter()) return false;

    if (!receipt.title.empty()) {
        if (!printer.printText(receipt.title)) return false;
        if (!printer.lineFeed()) return false;
    }

    if (!printer.alignLeft()) return false;

    for (const std::string& header : receipt.headers) {
        const auto lines = layout::wrapTextByDots(
            header,
            layoutConfig.contentWidthDots(),
            layoutConfig
        );

        for (const std::string& line : lines) {
            if (!printer.printText(line)) return false;
            if (!printer.lineFeed()) return false;
        }
    }

    if (!receipt.headers.empty()) {
        if (!printer.lineFeed()) return false;
    }

    for (const ReceiptItem& item : receipt.items) {
        const auto lines = layout::twoColumns(item.name, item.amount, layoutConfig);

        for (const auto& line : lines) {
            if (!printPositionedLine(line)) return false;
            if (!printer.lineFeed()) return false;
        }
    }

    if (!receipt.items.empty()) {
        if (!printer.lineFeed()) return false;
    }

    if (!printer.alignCenter()) return false;

    for (const std::string& footer : receipt.footers) {
        if (!printer.printText(footer)) return false;
        if (!printer.lineFeed()) return false;
    }

    if (!printer.lineFeed(3)) return false;

    return printer.cut();
}

bool ReceiptEngine::printPositionedLine(const layout::LayoutLine& line)
{
    int currentX = 0;
    std::string output;

    for (const auto& positionedText : line) {
        const int targetX = layoutConfig.paddingLeftDots + positionedText.xDots;
        const int gapDots = std::max(0, targetX - currentX);
        const int spaces = gapDots / layoutConfig.asciiCharWidthDots;

        output.append(static_cast<std::size_t>(spaces), ' ');
        output += positionedText.text;

        currentX = targetX + layout::textWidthDots(positionedText.text, layoutConfig);
    }

    return printer.printText(output);
}

} // namespace pe