#include "image_loader.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

template <typename T>
struct ComReleaser {
    void operator()(T* value) const { if (value) value->Release(); }
};

template <typename T>
using ComPtr = std::unique_ptr<T, ComReleaser<T>>;

std::wstring widen(const std::string& value)
{
    const int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0
    );
    if (size <= 0) return {};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), result.data(), size
    );
    return result;
}

} // namespace

bool loadImageAsMonochrome(
    const std::string& path,
    int requestedWidth,
    int threshold,
    LoadedImage& output,
    std::string& error
)
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninitialize = SUCCEEDED(comResult);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        error = "failed to initialize Windows image decoder";
        return false;
    }

    IWICImagingFactory* factoryRaw = nullptr;
    HRESULT result = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factoryRaw)
    );
    ComPtr<IWICImagingFactory> factory(factoryRaw);

    const std::wstring widePath = widen(path);
    IWICBitmapDecoder* decoderRaw = nullptr;
    if (SUCCEEDED(result) && !widePath.empty()) {
        result = factory->CreateDecoderFromFilename(
            widePath.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnLoad, &decoderRaw
        );
    }
    ComPtr<IWICBitmapDecoder> decoder(decoderRaw);

    IWICBitmapFrameDecode* frameRaw = nullptr;
    if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frameRaw);
    ComPtr<IWICBitmapFrameDecode> frame(frameRaw);

    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (SUCCEEDED(result)) result = frame->GetSize(&sourceWidth, &sourceHeight);
    if (FAILED(result) || sourceWidth == 0 || sourceHeight == 0) {
        if (uninitialize) CoUninitialize();
        error = "cannot decode image: " + path;
        return false;
    }

    const UINT targetWidth = requestedWidth > 0
        ? static_cast<UINT>(requestedWidth)
        : sourceWidth;
    const UINT targetHeight = std::max<UINT>(1, static_cast<UINT>(std::lround(
        static_cast<double>(sourceHeight) * targetWidth / sourceWidth
    )));

    IWICBitmapSource* source = frame.get();
    IWICBitmapScaler* scalerRaw = nullptr;
    ComPtr<IWICBitmapScaler> scaler;
    if (targetWidth != sourceWidth) {
        result = factory->CreateBitmapScaler(&scalerRaw);
        scaler.reset(scalerRaw);
        if (SUCCEEDED(result)) result = scaler->Initialize(
            frame.get(), targetWidth, targetHeight, WICBitmapInterpolationModeFant
        );
        source = scaler.get();
    }

    IWICFormatConverter* converterRaw = nullptr;
    if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converterRaw);
    ComPtr<IWICFormatConverter> converter(converterRaw);
    if (SUCCEEDED(result)) result = converter->Initialize(
        source, GUID_WICPixelFormat32bppBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom
    );

    const UINT stride = targetWidth * 4;
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(stride) * targetHeight
    );
    if (SUCCEEDED(result)) result = converter->CopyPixels(
        nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data()
    );
    if (FAILED(result)) {
        if (uninitialize) CoUninitialize();
        error = "cannot convert image: " + path;
        return false;
    }

    output.width = static_cast<int>(targetWidth);
    output.height = static_cast<int>(targetHeight);
    const std::size_t bytesPerRow = (targetWidth + 7) / 8;
    output.data.assign(bytesPerRow * targetHeight, 0);
    threshold = std::clamp(threshold, 0, 255);

    for (UINT y = 0; y < targetHeight; ++y) {
        for (UINT x = 0; x < targetWidth; ++x) {
            const std::size_t pixel = static_cast<std::size_t>(y) * stride + x * 4;
            const int b = pixels[pixel];
            const int g = pixels[pixel + 1];
            const int r = pixels[pixel + 2];
            const int a = pixels[pixel + 3];
            const int gray = (299 * r + 587 * g + 114 * b) / 1000;
            const int blended = (gray * a + 255 * (255 - a)) / 255;
            if (blended < threshold) {
                output.data[static_cast<std::size_t>(y) * bytesPerRow + x / 8] |=
                    static_cast<std::uint8_t>(0x80 >> (x % 8));
            }
        }
    }

    if (uninitialize) CoUninitialize();
    return true;
}
