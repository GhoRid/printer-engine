#ifndef RECEIPT_ENGINE_H
#define RECEIPT_ENGINE_H

#include "layout_engine.h"

#include <string>
#include <vector>

class Bixolon;

namespace pe {

struct ReceiptItem {
    std::string name;   // 항목명
    std::string amount; // 금액
};

struct ReceiptData {
    std::string title;                // 영수증 제목
    std::vector<std::string> headers; // 상단 정보
    std::vector<ReceiptItem> items;   // 출력 항목
    std::vector<std::string> footers; // 하단 정보
};

class ReceiptEngine
{
public:
    ReceiptEngine(Bixolon& printer, const layout::LayoutConfig& config = {});

    bool print(const ReceiptData& receipt);

private:
    Bixolon& printer;
    layout::LayoutConfig layoutConfig;

    bool printPositionedLine(const layout::LayoutLine& line);
};

} // namespace pe

#endif