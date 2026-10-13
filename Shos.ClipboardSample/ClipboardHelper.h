#pragma once

#include <afx.h>
#include <afxwin.h>
#include <atlimage.h>
#include <shlwapi.h>
#include <functional>

#pragma comment(lib, "shlwapi.lib")

class ClipboardHelper
{
    // Opens the clipboard and closes it on destruction (even when an exception is thrown).
    class ClipboardOpener
    {
        const bool isOpened;

    public:
        ClipboardOpener(CWnd& window) : isOpened(window.OpenClipboard() != FALSE)
        {}

        ~ClipboardOpener()
        {
            if (isOpened)
                ::CloseClipboard();
        }

        bool IsOpened() const
        {
            return isOpened;
        }

        ClipboardOpener(const ClipboardOpener&) = delete;
        ClipboardOpener& operator=(const ClipboardOpener&) = delete;
    };

public:
    static UINT GetClipboardFormat()
    {
        static const UINT clipboardFormat = ::RegisterClipboardFormat(_T("Shos.ClipboardSample.Figures"));
        return clipboardFormat;
    }

    static bool CanPaste()
    {
        return ::IsClipboardFormatAvailable(GetClipboardFormat()) != FALSE;
    }

    static bool Copy(CDocument& document, CWnd& view, CSize size, COLORREF backgroundColor, std::function<void(CDC&)> draw)
    {
        ClipboardOpener clipboardOpener(view);
        if (!clipboardOpener.IsOpened() || !::EmptyClipboard())
            return false;

        try {
            CopyMetaFileToClipboard(view, draw);
            CopyImageToClipboard(size, backgroundColor, draw);
            return CopyDataToClipboard(document);
        } catch (CException* exception) {
            exception->Delete();
            return false;
        }
    }

    static bool Paste(CWnd& view, std::function<void(CArchive&)> paste)
    {
        ClipboardOpener clipboardOpener(view);
        if (!clipboardOpener.IsOpened())
            return false;

        const HANDLE clipboardData = ::GetClipboardData(GetClipboardFormat());
        if (clipboardData == nullptr)
            return false;

        bool succeeded = false;
        CSharedFile sharedFile;
        sharedFile.SetHandle(clipboardData, FALSE);
        try {
            CArchive ar(&sharedFile, CArchive::load);
            paste(ar);
            ar.Close();
            succeeded = true;
        } catch (CException* exception) {
            exception->Delete();
        }
        // The memory is owned by the clipboard, so it must not be freed by sharedFile.
        sharedFile.Detach();
        return succeeded;
    }

private:
    static bool CopyMetaFileToClipboard(CWnd& view, std::function<void(CDC&)> draw)
    {
        CClientDC   clientDC(&view);
        CMetaFileDC metaDC;
        if (!metaDC.CreateEnhanced(&clientDC, nullptr, nullptr, nullptr))
            return false;
        draw(metaDC);
        const HENHMETAFILE enhancedMetaFileHandle = metaDC.CloseEnhanced();
        if (enhancedMetaFileHandle == nullptr)
            return false;

        // On success, the clipboard owns the handle.
        if (::SetClipboardData(CF_ENHMETAFILE, enhancedMetaFileHandle) == nullptr) {
            ::DeleteEnhMetaFile(enhancedMetaFileHandle);
            return false;
        }
        return true;
    }

    static bool CopyDataToClipboard(CDocument& document)
    {
        CSharedFile sharedFile;
        {
            CArchive ar(&sharedFile, CArchive::store);
            document.Serialize(ar);
        }
        const HGLOBAL dataHandle = sharedFile.Detach();

        // On success, the clipboard owns the handle.
        if (::SetClipboardData(GetClipboardFormat(), dataHandle) == nullptr) {
            ::GlobalFree(dataHandle);
            return false;
        }
        return true;
    }

    static bool CreateImage(CImage& image, CSize size, std::function<void(CDC&)> draw, COLORREF backgroundColor)
    {
        if (!image.Create(size.cx, size.cy, 24))
            return false;

        CDC* dc = CDC::FromHandle(image.GetDC());
        const CRect area(CPoint(), size);
        dc->FillSolidRect(&area, backgroundColor);
        draw(*dc);
        image.ReleaseDC();

        return true;
    }

    // Converts the image to a DIB (a BMP file without BITMAPFILEHEADER).
    static HGLOBAL ToDibHandle(const CImage& image)
    {
        CComPtr<IStream> stream;
        if (FAILED(::CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
            return nullptr;

        if (FAILED(image.Save(stream, Gdiplus::ImageFormatBMP)))
            return nullptr;

        ULARGE_INTEGER streamSize{};
        if (FAILED(::IStream_Size(stream, &streamSize)) || FAILED(::IStream_Reset(stream)))
            return nullptr;

        BITMAPFILEHEADER bitmapFileHeader{};
        if (streamSize.QuadPart <= sizeof(bitmapFileHeader) || FAILED(::IStream_Read(stream, &bitmapFileHeader, sizeof(bitmapFileHeader))))
            return nullptr;

        const auto    dibSize   = static_cast<ULONG>(streamSize.QuadPart - sizeof(bitmapFileHeader));
        const HGLOBAL dibHandle = ::GlobalAlloc(GHND, dibSize);
        if (dibHandle == nullptr)
            return nullptr;

        const auto    dib    = ::GlobalLock(dibHandle);
        const HRESULT result = dib == nullptr ? E_OUTOFMEMORY : ::IStream_Read(stream, dib, dibSize);
        if (dib != nullptr)
            ::GlobalUnlock(dibHandle);

        if (FAILED(result)) {
            ::GlobalFree(dibHandle);
            return nullptr;
        }
        return dibHandle;
    }

    static bool CopyImageToClipboard(CSize size, COLORREF backgroundColor, std::function<void(CDC&)> draw)
    {
        CImage image;
        if (!CreateImage(image, size, draw, backgroundColor))
            return false;

        const HGLOBAL dibHandle = ToDibHandle(image);
        if (dibHandle == nullptr)
            return false;

        // On success, the clipboard owns the handle.
        if (::SetClipboardData(CF_DIB, dibHandle) == nullptr) {
            ::GlobalFree(dibHandle);
            return false;
        }
        return true;
    }
};
