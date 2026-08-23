if (path == "/print/receipt") {
    // JSON 파싱

    ReceiptPrintData data;

    data.name = ...;
    data.offeringType = ...;
    data.amount = ...;

    return printReceipt(data);
}

if (path == "/print/access-pass") {
    AccessPassPrintData data;

    data.name = ...;
    data.department = ...;
    data.qrValue = ...;

    return printAccessPass(data);
}