#pragma once

#include <afx.h>
#include <functional>

class ClipboardHelper
{
public:
    static void CopyToClipboard(CSize size, std::function<void(CDC&)> draw)
    {
        if (::OpenClipboard(nullptr)) {
            ::EmptyClipboard();
            CopyImageToClipboard(size, draw);
            ::CloseClipboard();
        }
    }

private:
    static bool CreateImage(CImage& image, CSize size, std::function<void(CDC&)> draw, COLORREF color = RGB(0x00, 0x00, 0x00))
    {
        if (!image.Create(size.cx, size.cy, 24))
            return false;

        CDC* dc = CDC::FromHandle(image.GetDC());
        if (color != RGB(0x00, 0x00, 0x00)) {
            const CRect area(CPoint(), size);
            dc->FillSolidRect(&area, color);
        }
        draw(*dc);
        image.ReleaseDC();

        return true;
    }

    static HGLOBAL ToImageStream(const CImage& image)
    {
        IStream* stream = nullptr;
        HRESULT result = ::CreateStreamOnHGlobal(0, TRUE, &stream);
        if (!SUCCEEDED(result))
            return nullptr;

        result = image.Save(stream, Gdiplus::ImageFormatBMP);
        if (!SUCCEEDED(result)) {
            stream->Release();
            return nullptr;
        }

        ULARGE_INTEGER streamSize{};
        result = ::IStream_Size(stream, &streamSize);
        ASSERT(SUCCEEDED(result));

        const ULONG     streamLength = (ULONG)streamSize.LowPart;
        result = ::IStream_Reset(stream);
        ASSERT(SUCCEEDED(result));

        constexpr ULONG bitmapFileHeaderSize = (ULONG)sizeof(BITMAPFILEHEADER);
        const ULONG     actualStreamLength = streamLength - bitmapFileHeaderSize;
        const HGLOBAL   imageHandle = ::GlobalAlloc(GHND, actualStreamLength);
        if (imageHandle == nullptr) {
            stream->Release();
            return nullptr;
        }

        BYTE* byteArray = (BYTE*)::GlobalLock(imageHandle);
        result = ::IStream_Read(stream, byteArray, bitmapFileHeaderSize);
        ASSERT(SUCCEEDED(result));

        result = ::IStream_Read(stream, byteArray, actualStreamLength);
        ASSERT(SUCCEEDED(result));
        ::GlobalUnlock(imageHandle);
        stream->Release();

        return imageHandle;
    }

    static bool CopyImageToClipboard(CSize size, std::function<void(CDC&)> draw)
    {
        CImage image;
        CreateImage(image, size, draw, ::GetSysColor(COLOR_WINDOW));
        HGLOBAL imageStream = ToImageStream(image);
        if (imageStream == nullptr)
            return false;
        ::SetClipboardData(CF_DIB, imageStream);
        return true;
    }
};

