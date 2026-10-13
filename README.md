# [C++/MFC] Shos.ClipboardSample

Sample of copying and pasting to the clipboard.

* Clipboard operations:
    * Copy to:
      Extended metafiles, bitmaps, and custom format
    * Paste from:
      Custom format

* Development & Execution Environment
    * Language: C++
    * Framework: MFC
    * OS: Windows

* Build
    * Open `Shos.ClipboardSample.sln` in Visual Studio (platform toolset v145, C++20) and build.

* Usage
    * Figures > Random: adds 10 random figures (dots, lines, rectangles and ellipses).
    * Move the mouse to highlight the nearest figure, and click to select it.
    * Edit > Copy (Ctrl+C): copies the whole document to the clipboard as an enhanced metafile, a bitmap and the custom format "Shos.ClipboardSample.Figures".
    * Edit > Cut (Ctrl+X): copies the whole document and removes all figures.
    * Edit > Paste (Ctrl+V): adds the figures in the custom format to the document.
