# Shos.ClipboardSample リポジトリ全体レビュー

- レビュー日: 2026-10-13
- 対象: `main` ブランチ (HEAD: `7e1ccf6 Update Geometry.h`) + 作業ツリーの未コミット変更 (`Model.h`, `Shos.ClipboardSample.vcxproj`)
- 対象ファイル: `Shos.ClipboardSample/` 配下の全ソース (.h / .cpp 約 1,750 行)、プロジェクトファイル、リソース、README
- 方法: 静的なコードリーディングのみ (ビルド・実行による確認はしていない)

---

## 1. 総評

MFC の SDI アプリケーションで、図形 (点・線・矩形・楕円) を描画し、クリップボードへ **拡張メタファイル / DIB / 独自形式** でコピー、独自形式から貼り付けるサンプルです。

良い点:

- `Model` (データ) / `Document` (MFC との橋渡し) / `View` (描画) / `Command` (操作) の責務分離が意識されており、Observer パターンで Model の変更を View に伝える構成はサンプルとして分かりやすい。
- GDI オブジェクトの選択・復帰を RAII (`GdiObjectSelector`, `StockObjectSelector`) で行っており、選択し忘れによるリークを防いでいる。
- ダブルバッファリングを `DoubleBufferViewBase<TView>` としてテンプレート化し、`CView` / `CScrollView` の両方で使えるようにしている。
- 描画をラムダ (`std::function<void(CDC&)>`) で受け取ることで、画面・メタファイル・ビットマップの描画処理を共通化している。

一方で、**クリップボード処理の堅牢性**、**ダブルバッファのキャッシュ無効化漏れによる表示不整合**、**生ポインタによる所有権管理** に重要な問題があります。また、**ローカルの `.git` オブジェクトが破損している** ことを確認しました (§2.0)。

### 重要度の凡例

| 記号 | 意味 |
|---|---|
| 🔴 High | 実害のあるバグ・クラッシュ・データ破損の可能性 |
| 🟠 Medium | 条件次第で不具合になる / 設計上の明確な問題 |
| 🟡 Low | 保守性・可読性・軽微な非効率 |

---

## 2. 指摘事項 (重要度順)

### 2.0 🔴 ローカル Git リポジトリのオブジェクト破損

`git diff` / `git fsck` 実行時に以下のエラーが発生しました。

```
error: inflate: data stream error (unknown compression method)
error: unable to unpack 81b45a2151a7c156dcad450b8a05a8a632727fa8 header
fatal: loose object 5c76f698d00e12f021501f6431e3356506c400c8 (...) is corrupt
error: 011b40f5..., 04155d02..., 04551277... : object corrupt or missing
```

- リポジトリが Dropbox 配下 (`/mnt/c/dropbox/...`) にあり、同期処理 (オンライン専用ファイル化・同時書き込みなど) で loose object が壊れた可能性が高いと考えられます。
- このため、未コミット変更 (`Model.h`, `.vcxproj`) の差分は確認できず、作業ツリーの現状のみをレビューしました。
- **推奨**: 作業ツリーの変更を退避したうえで、GitHub から Dropbox 外のディレクトリに再 clone し、変更を戻す。今後は Git リポジトリを同期フォルダーの外に置く。

---

### 2.1 🔴 貼り付け・切り取り後に画面が更新されない (キャッシュ無効化漏れ)

**該当**: `ClipboardHelper.h:25-31, 33-43, 80-95` / `DoubleBuffer.h:54-57, 70-97` / `Model.h:109-131`

`View` はダブルバッファのキャッシュ (`isCashValid`) を `OnUpdate()` / `OnSize()` / スクロールでのみ無効化します。ところが:

- **Paste**: `AddDataFromClipboard()` は `document.Serialize(ar)` → `Model::Serialize()` で `figures` に直接追加するだけで、Observer への通知 (`Update(hint)`) も `UpdateAllViews()` も行いません。その後 `view.Invalidate()` を呼んでも、キャッシュは有効なままなので **古いビットマップが BitBlt されるだけ** で、貼り付けた図形は表示されません (ウィンドウサイズ変更などで初めて表示される)。
- **Cut**: `document.DeleteContents()` → `Model::Clear()` も通知しないため、**削除した図形が画面に残り続けます**。さらに `SetModifiedFlag()` も呼ばれないため、切り取り後に保存確認が出ません。

**推奨**:

- `Model` に「貼り付け (追加)」「全削除」用の公開メソッドを用意し、その中で `Hint` を発行する。例:

  ```cpp
  void Model::Load(CArchive& ar)   // 貼り付け用
  {
      std::vector<Figure*> loaded;
      /* ReadObject して loaded と figures に追加 */
      Hint hint(Hint::Type::Added, loaded);
      Update(hint);
  }
  ```

- `ClipboardHelper` は `CDocument&` ではなく、上記の操作を呼べるインターフェイス (または `Document&`) を受け取るようにする。`DeleteContents()` は MFC フレームワークが「新規作成 / 開く / 閉じる」時に呼ぶためのもので、編集操作から直接呼ぶのは避けるべきです。

---

### 2.2 🔴 独自クリップボード形式に `CF_PRIVATEFIRST` を固定使用している

**該当**: `ClipboardHelper.h:68-78, 80-95`

`CF_PRIVATEFIRST` は「どのアプリも自由に使ってよい私的な範囲」であり、**他のアプリが同じ値に全く別のデータを置く可能性があります**。その状態で Paste すると:

1. 他アプリのデータを `CArchive` で読み込もうとして `CArchiveException` が送出される。
2. 例外が `ClipboardHelper::OnEditPaste()` 内で捕捉されないため、`::CloseClipboard()` が呼ばれず、**クリップボードが開いたまま** になる (他アプリがクリップボードを使えなくなる)。
3. 最終的に MFC の既定の例外処理でメッセージが出る、または状態不整合になる。

また、プライベート範囲のハンドルはシステムがグローバルメモリとして扱わないため、**別プロセス (アプリの 2 つ目のインスタンスなど) への貼り付けは正しく動作しない可能性が高い** です。

**推奨**:

```cpp
static UINT GetClipboardFormat()
{
    static const UINT format = ::RegisterClipboardFormat(_T("Shos.ClipboardSample.Figures"));
    return format;
}
```

- `RegisterClipboardFormat` で登録した形式を使えば、データはシステムが管理・解放するため、`globalMemoryHandle` と `OnDestroyClipboard()` (`WM_DESTROYCLIPBOARD` 処理) が丸ごと不要になります。
- 読み込み側は `try { ... } catch (CException* e) { e->Delete(); }` で囲み、`CloseClipboard()` を必ず呼ぶ (RAII のクリップボードガードクラスを作るのが望ましい)。同様に `OnEditCopy()` も例外時にクリップボードが開きっぱなしになる。

---

### 2.3 🟠 `OnDestroyClipboard()` のロック/解放処理の問題

**該当**: `ClipboardHelper.h:45-53`

- `CSharedFile::Detach()` はメモリのロックを解除して返すため、`::GlobalUnlock()` は不要 (ロックカウント 0 のハンドルに対する呼び出し)。
- アプリ終了時、クリップボードにデータを残したままウィンドウが破棄されると、`globalMemoryHandle` は解放されないまま残る。
- 2.2 の対応 (`RegisterClipboardFormat`) を行えば、この処理自体を削除できます。

---

### 2.4 🟠 楕円の距離計算が不正確で、内部では負の値を返す

**該当**: `Geometry.h:69-83` / `Figure.h:342-345`

```cpp
auto distance = Abs(d) - rect.Height() / 2;
distance = Round(distance / rate);
```

1. **内部で負になる**: 楕円の中心付近では `-(Width/2)` 程度の負値になります。`SelectCommand::GetNearestFigure()` は最小距離の図形を選ぶため、**楕円の内側にカーソルがあると、すぐ近くを通る線分よりも常に楕円が優先されます**。`DotFigure` は 0 でクランプ、`RectangleFigure` は辺までの距離 (非負) を返しており、図形間で距離の意味が一貫していません。
2. **短軸方向の距離が拡大される**: 例えば幅 1000・高さ 100 の楕円で、上端から 10 離れた点は `10 / 0.1 = 100` と計算されます。最後の `/ rate` は長軸方向の補正にしかなっていません。

**推奨**: 少なくとも `std::abs()` を取り、輪郭線までの距離 (非負) を返す。より正確にするなら、楕円上の点を角度で数十分割した折れ線への距離で近似する方法が簡単です。

---

### 2.5 🟠 最近傍図形の探索が探索距離を超えた図形も選択する

**該当**: `Command.h:98-117`

探索矩形と図形の **外接矩形 (`GetArea()`)** が交差するかだけで候補にしており、`figureDistance <= searchingDistance` の判定がありません。対角線方向の長い線分や大きな矩形の場合、外接矩形の内側であれば **線から数百離れた位置でもハイライト・選択されます**。

**推奨**: `if (figureDistance <= searchingDistance && figureDistance < distance)` とする。

---

### 2.6 🟠 生ポインタによる所有権管理と、コピー時の二重解放

**該当**: `Model.h:37, 49-52, 71-82, 125-131` / `Command.h:122-143` / `Figure.h:358-363`

- `Model` は `std::vector<Figure*>` を所有してデストラクタで `delete` していますが、コピーコンストラクタ/代入が暗黙に生成されるため、**コピーされると二重解放** になります。`CommandManager` (`Command* currentCommand`) も同様です。
- `Model::Change()` は差し替えた `oldFigure` を解放せず、所有権が曖昧 (呼び出し側の責任なのか不明。現状未使用)。また `highlightedFigure` が `oldFigure` を指していた場合、ダングリングポインタになります。
- `CommandManager::SetCommand(nullptr)` を呼ぶと `currentCommand->Set(model)` で null 参照になります。

**推奨**: `std::vector<std::unique_ptr<Figure>>` / `std::unique_ptr<Command>` を使う。少なくともコピーを `= delete` する。

---

### 2.7 🟠 `Observable::RemoveObserver()` が要素を削除しない

**該当**: `Observer.h:23-26`

```cpp
std::remove(observers.begin(), observers.end(), &observer);
```

`std::remove` は要素を末尾に寄せるだけで、`erase` しないとサイズは変わりません。現状は未使用ですが、使った瞬間に **削除済みの Observer に通知してクラッシュ** する潜在バグです (C++20 では `[[nodiscard]]` 警告対象)。

**推奨**: `std::erase(observers, &observer);` (C++20) または erase-remove イディオム。あわせて `Observer` に `virtual ~Observer() = default;` を追加し、`<algorithm>` を include する。

---

### 2.8 🟠 クラス定義内での修飾名 `View::OnInitialUpdate`

**該当**: `View.h:28`

```cpp
virtual void View::OnInitialUpdate() override
```

クラス定義内でメンバー関数宣言に `View::` を付けるのは C++ として不正です (MSVC の拡張で通っているだけ)。`SCROLL_VIEW` を有効にし、`/permissive-` (準拠モード) でビルドするとエラーになります。

**推奨**: `virtual void OnInitialUpdate() override` とする。

---

### 2.9 🟠 Win32 構成と x64 構成でビルド設定が不一致

**該当**: `Shos.ClipboardSample.vcxproj`

- `LanguageStandard` (`stdcpp20`) が x64 の Debug/Release にのみ設定され、Win32 構成は既定 (C++14) のまま。
- `ConformanceMode` (`/permissive-`) が未設定。2.8 のような非標準コードを検出できません。
- `WarningLevel` が `Level3`。`Level4` にすると、未使用引数・符号比較 (`Geometry.h:107`) などを検出できます。

**推奨**: 全構成で `stdcpp20` / `ConformanceMode=true` / `Level4` に揃える。

---

### 2.10 🟡 クリップボード処理のその他の問題

**該当**: `ClipboardHelper.h`

| 行 | 内容 |
|---|---|
| 33-43 | `OnEditPaste()` は貼り付けるデータが無くても `true` を返す。`SetModifiedFlag()` が 38 行目と 93 行目で二重に呼ばれている。 |
| 56-66 | `document` 引数が未使用。`CreateEnhanced()` の失敗チェックが無い。`CopyEnhMetaFile()` で複製してから元を削除しているが、元のハンドルをそのまま `SetClipboardData()` に渡せば十分。 |
| 87 | `SetHandle(clipboardData)` は既定で `bAllowGrow = TRUE`。クリップボード所有のメモリなので `FALSE` を指定すべき。 |
| 97-111 | 「背景色が黒なら塗りつぶさない」というセンチネル値の使い方が分かりにくい。常に塗りつぶせばよい。 |
| 154-163 | `CreateImage()` の戻り値を無視しており、失敗時に空の `CImage` を `Save()` してしまう。 |
| 128-147 | `IStream_Size` / `IStream_Read` の結果を `ASSERT` のみで確認しており、Release ビルドではエラーを検出できない。 |
| 143 | ヘッダー読み飛ばしのために出力バッファを一時領域として使っている。`IStream_Seek` などで読み飛ばす方が意図が明確。 |

また、`ON_UPDATE_COMMAND_UI` が無いため、**クリップボードに対応データが無くても「貼り付け」が常に有効** です。`::IsClipboardFormatAvailable()` でメニューの有効/無効を切り替えることを推奨します。

---

### 2.11 🟡 `View` の実装

**該当**: `View.h`

- **24 行**: `reinterpret_cast<Document&>` → `static_cast` または `STATIC_DOWNCAST(Document, m_pDocument)` を使うべき。
- **73, 84 行**: `pHint` を無条件に `static_cast<Hint*>` している。`Hint` に `DECLARE_DYNAMIC` を付けて `DYNAMIC_DOWNCAST` で確認する方が安全。
- **83-90 行**: 部分再描画のために変更領域だけ `InvalidateRect()` しているが、直前の `Update()` でキャッシュ全体を無効化しているため、次の描画でキャッシュは全体が再描画される。部分無効化の効果は BitBlt の範囲だけで、意図した最適化になっていない。`FigureHelper::GetArea()` の戻り値 (空の場合 `false`) も無視されている。
- **113-117 行**: `WM_MOUSEMOVE` ごとにウィンドウ全体を `Invalidate()` し、さらに `DPtoLP()` 内で `CClientDC` を毎回生成している。カーソル付近と旧ハイライト領域だけを無効化すれば十分。
- マウスがウィンドウ外に出てもカーソル十字が残る (`WM_MOUSELEAVE` 未処理)。
- `OnDrawLayer1` / `OnDrawLayer2` に `override` が無い。
- 選択・ハイライトの判定は `Document::IsValid()` で図形領域内に限定されているため、領域外をクリックしても選択解除できない。

---

### 2.12 🟡 `DoubleBuffer` の実装

**該当**: `DoubleBuffer.h`

- `CashDevice` クラス (25-47 行) は未使用で、内部で `new` / `delete` を使っている。削除を推奨。
- `isCashValid`, `cash`, `DrawCash` などの「Cash」は「Cache」の誤り。
- `DrawCash()` で `CreateCompatibleBitmap()` の失敗 (クライアント領域 0、リソース不足など) を確認していない。
- `OnDraw()` が `ASSERT(false)` のため、印刷・印刷プレビューを追加した場合に動作しない。少なくともコメントで理由を明記する。
- `OnEraseBkgnd()` が `BOOL` に `true` を返している (`TRUE` が慣例)。コメントアウトされたコードも削除可。
- 仮想関数 `OnPaint(CDC&)` とメッセージハンドラ `OnPaint()` が同名のオーバーロードで紛らわしい。

---

### 2.13 🟡 `Figure` / `FigureHelper` の実装

**該当**: `Figure.h`

- **143 行**: `DotFigure::radius` が非 static の `const` メンバー。インスタンスごとに領域を取り、代入演算子も削除される。`static constexpr long radius = 10L;` にすべき。
- **189, 245, 286 行**: `GetPoints()` のオーバーライドに `override` が無い。
- **314, 342 行**: `RectangleFigure` / `EllipseFigure` の `GetDistanceFrom()` が `protected` になっている (基底では `public`)。仮想呼び出しでは動くが、アクセス指定が一貫していない。
- **62 行**: `Clone()` は `const` にすべき。`RectangleFigureBase` には `Clone()` が無く、基底の `Figure::Clone()` が使われてしまう。
- **409-412 行**: `mt() % range` はモジュロバイアスがあり、`unsigned` と `long` の混在演算にもなっている。`std::uniform_int_distribution<long>` を使う。引数名 `maxmum` は `maximum` の誤記。
- **394-399 行**: `default` で `figure = nullptr` にした直後に `figure->Attribute()` を参照しており、Release ビルドで到達すると null 参照。
- **366 行**: `GetArea(std::vector<Figure*> figures, ...)` は値渡し。`const&` にする。
- **9-11 行 / `Geometry.h:6-8`**: `#undef max` で `windows.h` のマクロを消しているが、ヘッダーの include 順に依存する。`NOMINMAX` をプロジェクト全体で定義するか、`(std::numeric_limits<long>::max)()` と書く。
- `<algorithm>` / `<vector>` を include せずに `std::for_each` / `std::transform` を使っている (間接 include 依存)。
- 「Selecter」→「Selector」、`Hilight`→`Highlight` などの綴り誤り。

---

### 2.14 🟡 `Geometry` の実装

**該当**: `Geometry.h`

- `<cmath>` を include せずに `floor` / `sqrt` を使用。`abs` も `std::abs` を使うべき。
- `Round()` は `std::lround()` で置き換え可能。
- **101-113 行**: 先頭要素 `areas[0]` だけ `NormalizeRect()` されない。ループ変数 `auto index = 1` (int) と `size_t` の比較で符号不一致警告。
- 内積・外積を `long` (Windows では 32bit) で計算しており、現状の座標範囲 (0〜2000) では問題ないが、座標範囲を広げるとオーバーフローし得る。

---

### 2.15 🟡 `Document` / `Model` / `Command` のその他

- `Document::Add()` (`Document.h:34-38`) は `model.Add()` 内の通知でも `SetModifiedFlag()` されるため二重。また `Document::Add()` は未使用。
- `Model::Serialize()` (`Model.h:118`) は `ReadObject(NULL)` を使っており型チェックが無い。`ReadObject(RUNTIME_CLASS(Figure))` とすれば、想定外のクラスを読んだ時点で例外になる。
- `Model::Hilight()` はハイライト変更を通知しないため、View 側が毎回の `WM_MOUSEMOVE` で全体再描画することに依存している (2.11)。
- `SelectCommand::OnMouseMove()` (`Command.h:89`) に `override` が無い。`sizeof(points) / sizeof(CPoint)` は `std::size(points)` で書ける。コメントアウトされたメンバー (`//Figure* nearestFigure;`) は削除可。
- `Hint` は `CObject` 派生だが `DECLARE_DYNAMIC` が無いため実行時型情報を持たない (2.11)。
- `Observer::Update(THint&)` は通知内容を変更しないので `const THint&` が望ましい。

---

### 2.16 🟡 アプリケーション / ビルド構成

- `Application.cpp:25` の `if (!pDocTemplate)` は、`new` が失敗時に例外を投げるため無意味。
- `Application.cpp:3-4` の `afxwinappex.h` / `afxdialogex.h`、`framework.h:23` の `afxcontrolbars.h` は未使用。`framework.h` 末尾の空行も削除可。
- `ClipboardHelper.cpp`, `Command.cpp`, `Figure.cpp`, `DoubleBuffer.cpp` 以外の .cpp では `DEBUG_NEW` を定義しているが、これらでは定義しておらず一貫していない (ヘッダーに実装があるためリーク検出のファイル名も不正確になる)。
- `.rc` が UTF-16 のため、Git の差分が表示されない。`.gitattributes` に `*.rc diff working-tree-encoding=UTF-16LE-BOM eol=CRLF` などを設定するか、UTF-8 で保存すると差分レビューしやすくなる。
- インデントにタブとスペースが混在している (`ClipboardHelper.h`, `DoubleBuffer.h`, `Geometry.h` など)。`.editorconfig` の追加を推奨。

---

### 2.17 🟡 ドキュメント / テスト

- README にビルド手順 (Visual Studio のバージョン、`v145` ツールセット) と操作方法 (Figures > Random で図形生成、マウスで選択、Ctrl+C/X/V) を追記すると、サンプルとして使いやすくなります。
- 「コピーはドキュメント全体」「選択図形のみではない」という仕様も README に明記するとよいでしょう (現在の `Select` 状態はコピー対象に影響しない)。
- `Geometry` は MFC 依存が `CPoint` / `CRect` だけなので、距離計算 (特に 2.4 の楕円) について単体テスト (Microsoft C++ Unit Test Framework / GoogleTest) を追加する価値があります。

---

## 3. 推奨する対応順序

1. **Git リポジトリの復旧** (§2.0) — 作業内容を失わないために最優先。
2. **クリップボード形式の登録と例外安全化** (§2.2, §2.3) — 他アプリとの干渉・クリップボードのロックを防ぐ。
3. **Paste / Cut 後の通知とキャッシュ無効化** (§2.1) — 目に見える不具合。
4. **距離計算と選択ロジックの修正** (§2.4, §2.5)。
5. **所有権の `unique_ptr` 化、`RemoveObserver` 修正** (§2.6, §2.7)。
6. ビルド設定の統一 (§2.8, §2.9) と、その他の 🟡 項目。
