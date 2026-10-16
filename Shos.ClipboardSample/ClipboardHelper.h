#pragma once

#include <afx.h>
#include <afxwin.h>
#include <atlimage.h>
#include <shlwapi.h>
#include <concepts>

#pragma comment(lib, "shlwapi.lib")

// Copies the document to the clipboard and pastes figures from it.
// Copy puts the following formats on the clipboard at once:
//   - CF_ENHMETAFILE: a vector image, for applications such as Word or PowerPoint
//   - CF_DIB        : a bitmap image, for applications such as Paint
//   - an application-specific registered format: the serialized figures, for pasting back into this application
class ClipboardHelper
{
    // Opens the clipboard and closes it on destruction (even when an exception is thrown).
    class ClipboardOpener
    {
        const bool isOpened;

    public:
        explicit ClipboardOpener(CWnd& window) : isOpened(window.OpenClipboard() != FALSE)
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
    // Returns the application-specific clipboard format (registered on the first call).
    static UINT GetClipboardFormat()
    {
        static const UINT clipboardFormat = ::RegisterClipboardFormat(_T("Shos.ClipboardSample.Figures"));
        return clipboardFormat;
    }

    // Returns true when the clipboard holds data in the application-specific format.
    static bool CanPaste()
    {
        return ::IsClipboardFormatAvailable(GetClipboardFormat()) != FALSE;
    }

    // Copies the document to the clipboard in all the formats above.
    // 'draw' draws the document on the given DC; 'size' and 'backgroundColor' are used for the bitmap.
    // The image formats are optional extras, so the result tells only whether the application data was copied.
    static bool Copy(CDocument& document, CWnd& view, CSize size, COLORREF backgroundColor, std::invocable<CDC&> auto&& draw)
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

    // Reads the application-specific data from the clipboard and passes it to 'paste' as a loading archive.
    // Returns false when there is no such data or it could not be read.
    static bool Paste(CWnd& view, std::invocable<CArchive&> auto&& paste)
    {
        ClipboardOpener clipboardOpener(view);
        if (!clipboardOpener.IsOpened())
            return false;

        const HANDLE clipboardData = ::GetClipboardData(GetClipboardFormat());
        if (clipboardData == nullptr)
            return false;

        bool succeeded = false;
        // Wrap the clipboard memory in a CSharedFile so that it can be read through CArchive.
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
    // Records the drawing in an enhanced metafile and puts it on the clipboard as CF_ENHMETAFILE.
    static bool CopyMetaFileToClipboard(CWnd& view, std::invocable<CDC&> auto& draw)
    {
        // The client DC of the view is the reference device of the metafile.
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

    // Serializes the document into global memory and puts it on the clipboard in the application-specific format.
    static bool CopyDataToClipboard(CDocument& document)
    {
        CSharedFile sharedFile;
        {
            // The archive is closed at the end of this scope, which flushes its buffer to the file before Detach.
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

    // Creates a 24-bit bitmap of the given size, fills it with the background color and draws on it.
    static bool CreateImage(CImage& image, CSize size, std::invocable<CDC&> auto& draw, COLORREF backgroundColor)
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
        // Save the image as a BMP file into a memory stream.
        CComPtr<IStream> stream;
        if (FAILED(::CreateStreamOnHGlobal(nullptr, TRUE, &stream)))
            return nullptr;

        if (FAILED(image.Save(stream, Gdiplus::ImageFormatBMP)))
            return nullptr;

        // Skip BITMAPFILEHEADER. The rest (BITMAPINFOHEADER, color table and pixels) is the DIB.
        ULARGE_INTEGER streamSize{};
        if (FAILED(::IStream_Size(stream, &streamSize)) || FAILED(::IStream_Reset(stream)))
            return nullptr;

        BITMAPFILEHEADER bitmapFileHeader{};
        if (streamSize.QuadPart <= sizeof(bitmapFileHeader) || FAILED(::IStream_Read(stream, &bitmapFileHeader, sizeof(bitmapFileHeader))))
            return nullptr;

        // Copy the DIB into movable global memory, which is what the clipboard requires.
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

    // Draws the document on a bitmap and puts it on the clipboard as CF_DIB.
    static bool CopyImageToClipboard(CSize size, COLORREF backgroundColor, std::invocable<CDC&> auto& draw)
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
