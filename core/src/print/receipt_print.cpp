#include "receipt_print.h"

#include "../bitmap.h"
#include "../layout_engine.h"

bool printReceipt(const ReceiptPrintData& data)
{
    // 여기에서 영수증 UI를 직접 구성

    // 예:
    // Bitmap bitmap(576, ...);
    //
    // drawText(bitmap, "헌금 영수증", ...);
    // drawText(bitmap, data.name, ...);
    // drawText(bitmap, data.offeringType, ...);
    // drawText(bitmap, std::to_string(data.amount), ...);
    //
    // drawImage(...)
    // drawLine(...)
    //
    // printer로 bitmap 전송

    return true;
}