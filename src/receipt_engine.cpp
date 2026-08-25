#include "receipt_engine.h"
#include "printer/printer_backend.h"

#include <algorithm>
#include <string>

namespace pe {

ReceiptEngine::ReceiptEngine(PrinterBackend& printer, const layout::LayoutConfig& config)
    : printer(printer), layoutConfig(config)
{
}

bool ReceiptEngine::print(const ReceiptData& receipt)
{
    if (!printer.alignLeft()) return false;

    if (!receipt.title.empty()) {
        const auto lines = layout::alignedText(
            receipt.title,
            layout::TextAlignment::Center,
            layoutConfig
        );
        for (const auto& line : lines) {
            if (!printPositionedLine(line)) return false;
            if (!printer.lineFeed()) return false;
        }
    }

    for (const std::string& header : receipt.headers) {
        const auto lines = layout::alignedText(
            header,
            layout::TextAlignment::Left,
            layoutConfig
        );

        for (const auto& line : lines) {
            if (!printPositionedLine(line)) return false;
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

    for (const std::string& footer : receipt.footers) {
        const auto lines = layout::alignedText(
            footer,
            layout::TextAlignment::Center,
            layoutConfig
        );
        for (const auto& line : lines) {
            if (!printPositionedLine(line)) return false;
            if (!printer.lineFeed()) return false;
        }
    }

    if (!printer.lineFeed(3)) return false;

    return printer.cut();
}

bool ReceiptEngine::printPositionedLine(const layout::LayoutLine& line)
{
    for (const auto& positionedText : line) {
        const int targetX = layoutConfig.paddingLeftDots + positionedText.xDots;
        if (!printer.setAbsolutePosition(targetX) ||
            !printer.printText(positionedText.text)) return false;
    }

    return true;
}

} // namespace pe
