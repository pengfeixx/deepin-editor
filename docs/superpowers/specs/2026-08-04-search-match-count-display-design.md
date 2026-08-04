# 搜索匹配计数显示 设计文档

- 日期：2026-08-04
- 范围：编辑器搜索栏（FindBar）与替换栏（ReplaceBar）左侧"查找"输入框
- 目标：在搜索输入框内、清除按钮左侧（间距 6px）显示当前匹配项索引与匹配总数，格式 `第xx/xx项`（如 `第5/10项`）

## 1. 背景

当前搜索流程：
- 用户在 FindBar 输入关键字并回车 → `FindBar::handleContentChanged` → 发出 `updateSearchKeyword` 信号
- `Window::handleUpdateSearchKeyword`（src/widgets/window.cpp:3756）→ `TextEdit::highlightKeyword`（src/editor/dtextedit.cpp:2305）
- `highlightKeyword` 内部调用：
  - `updateCursorKeywordSelection`（dtextedit.cpp:2345）跳转到匹配项
  - `updateKeywordSelectionsInView`（dtextedit.cpp:2422）高亮**仅可视区域**内的匹配
- Next/Prev 导航：`Window::handleFindNextSearchKeyword` / `handleFindPrevSearchKeyword`（window.cpp:3567/3574）→ `TextEdit::updateCursorKeywordSelection`

关键约束：
- 现有 `m_findMatchSelections` 只保存**可视区域**内的匹配项，不能直接用作"全文档总数"
- `TextEdit::updateKeywordSelections`（dtextedit.cpp:2385）已实现全文档扫描逻辑但当前未被使用，可作为蓝本
- 没有现成的"当前是第几个匹配"概念，需根据高亮光标位置在全文档匹配列表中定位

清除按钮来源：`LineBar`（src/controls/linebar.cpp:21）继承自 `DLineEdit`，通过 `setClearButtonEnabled(true)` 启用内置清除按钮（位于输入框右侧内部）。

## 2. 需求

1. 在 FindBar 的输入框内输入任意字符并回车后，显示 `第xx/xx项`
2. 点击"上一个/下一个"按钮导航时，索引数字实时更新
3. ReplaceBar 左侧"Find"输入框（`m_replaceLine`）同样显示；右侧"Replace With"输入框（`m_withLine`）不显示
4. 显示位置：输入框内、清除按钮左侧，间距 6px
5. 无任何匹配时隐藏计数（保留现有红色 `setMismatchAlert` 警示）
6. 当前光标未落在任何匹配项时（total>0 但 current=0）显示 `第0/N项`，表示未定位到匹配

## 3. 方案

采用 **方案 B：自绘清除按钮 + DLineEdit::setRightWidgets**（方案 A 在 Qt6 下被证伪后切换）。

**方案 A 失败原因（实测）**：Qt6 下 `QLineEdit::addAction(TrailingPosition)` 把自定义 action 放在内置清除按钮**右侧**（不是文档假设的左侧）；`DLineEdit::setRightWidgets` 同样把自定义 widget 放在内置清除按钮右侧。两种 API 都无法让自定义 widget 位于内置清除按钮左侧。

**方案 B 做法**：禁用内置清除按钮（`setClearButtonEnabled(false)`），自绘一个 `DIconButton(QStyle::SP_LineEditClearButton)`，与计数 label 放入同一容器 `[label][6px spacer][清除按钮]`，通过 `DLineEdit::setRightWidgets` 添加。容器内部布局完全可控，label 在清除按钮左侧、间距 6px。

- 清除按钮：`DIconButton(QStyle::SP_LineEditClearButton)`，16x16，点击清空文本，可见性跟随文本非空
- 6px 间距：容器内 `QHBoxLayout`（contentsMargins=0, spacing=0），`addWidget(label)` + `addSpacing(6)` + `addWidget(clearButton)`
- label 右边缘距清除按钮左侧 = 6px（由 spacer 保证）

## 4. 架构

数据流：

```
回车 / 点击 Next / 点击 Prev
   ↓
TextEdit::highlightKeyword / updateCursorKeywordSelection（现有流程，末尾插入新调用）
   ↓
TextEdit::updateMatchCount（新增）
   ├── 缓存命中：复用 m_allMatchPositions
   └── 缓存失效：全文档重扫
   ↓ 计算 current（二分查找）+ total
TextEdit emit findMatchCountChanged(int current, int total)（新增信号）
   ↓ Window 在 per-wrapper 连线块中转发
FindBar::slotUpdateMatchCount / ReplaceBar::slotUpdateMatchCount（新增槽）
   ↓
LineBar::setMatchCount(current, total)
   ↓
QLabel setText("第5/10项")；total==0 时 hide
```

### 4.1 TextEdit 层（数据计算）

文件：`src/editor/dtextedit.h` / `src/editor/dtextedit.cpp`

**接口设计原则**：把"扫描"和"索引查找"拆成纯函数（public、const、无副作用、返回值直接可断言），把"缓存判断 + emit"留给私有编排函数。这样核心逻辑无需依赖 `-fno-access-control` 即可单测，且测试不依赖 QSignalSpy 间接验证。

新增成员（私有）：
- `QList<int> m_allMatchPositions` — 全文档匹配的起始位置列表（升序），缓存
- `QString m_countedKeyword` — 缓存对应的关键字
- `Qt::CaseSensitivity m_countedCase` — 缓存对应的大小写标志

新增信号：
- `void findMatchCountChanged(int current, int total)`

新增 **public** 方法（纯函数，直接单测）：
- `QList<int> scanAllMatchPositions(const QString &keyword, Qt::CaseSensitivity caseFlag) const`
  - 参考 `updateKeywordSelections`（dtextedit.cpp:2385）的扫描逻辑，对全文档自首至尾用 `QTextDocument::find` 推进，收集每个匹配 `QTextCursor` 的 `selectionStart()`，返回升序列表
  - 空关键字返回空列表
- `int findCurrentMatchIndex() const`
  - 读取 `m_allMatchPositions` 缓存与 `m_findHighlightSelection.cursor.selectionStart()`，二分查找相等项；返回 **1-based** 索引，未命中返回 0
  - 缓存为空时返回 0
- `void invalidateMatchCountCache()`（**public**，非 private）
  - `m_countedKeyword.clear(); m_allMatchPositions.clear();`
  - 设为 public 的原因：`Window::addTabWithWrapper` 在 tab 跨窗口拖拽时会调用 `wrapper->textEditor()->disconnect()`（无参形式，切断所有信号含自连接），随后需重新建立 `textChanged → invalidateMatchCountCache` 的自连接；connect 取成员函数地址要求该函数可访问。该函数语义为无害的清缓存操作（不 emit、不改文档），公开风险低。`updateMatchCount`（emit 信号、有副作用）仍保持 private，调用点单一。

新增 **private** 方法（编排 + emit）：
- `void updateMatchCount(const QString &keyword, Qt::CaseSensitivity caseFlag)`
  1. 空关键字：`emit findMatchCountChanged(0, 0); return;`（早期返回，避免缓存被空关键字污染）
  2. 若 `keyword != m_countedKeyword` 或 `caseFlag != m_countedCase` → `m_allMatchPositions = scanAllMatchPositions(keyword, caseFlag);` 刷新缓存与标记
  3. `total = m_allMatchPositions.size()`；`current = findCurrentMatchIndex()`
  4. `emit findMatchCountChanged(current, total)`
- `void invalidateMatchCountCache()` — 已移至 public（见上文说明）

调用点（**仅一处**，避免双重 emit）：
- `updateCursorKeywordSelection`（dtextedit.cpp:2345）末尾调用 `updateMatchCount(keyword, caseFlag)`
- **不在** `highlightKeyword`（dtextedit.cpp:2305）中调用——因为它内部已经调用 `updateCursorKeywordSelection`，两处都加会导致回车搜索时信号发两次
- 这一处调用同时覆盖三条路径：回车首次搜索（经 `highlightKeyword` → `updateCursorKeywordSelection`）、Next、Prev

缓存失效（**单一入口**，简化设计）：
- 仅在 TextEdit 构造函数内连接：`connect(this, &QPlainTextEdit::textChanged, this, &TextEdit::invalidateMatchCountCache);`
- 理由：替换命令（`ReplaceAllCommand` 等）修改文档文本时已会触发 `QPlainTextEdit::textChanged`，自动失效缓存；`removeKeywords` 不改文档文本，缓存的位置列表依然有效，无需失效
- 删除 spec 原稿中"在 `removeKeywords` / 替换命令类中显式调用"的冗余设计

### 4.2 UI 层（LineBar / FindBar / ReplaceBar）

#### LineBar（封装计数 label，共用）

文件：`src/controls/linebar.h` / `src/controls/linebar.cpp`

新增成员（私有）：
- `QLabel *m_matchCountLabel`
- `DIconButton *m_clearButton`（自绘清除按钮，替代 DLineEdit 内置的）

新增公共方法：
- `void setMatchCount(int current, int total)`
  - `total == 0` → `m_matchCountLabel->hide()`
  - 否则 `setText(QString("第%1/%2项").arg(current).arg(total))`，`show()`

构造函数内：
- `setClearButtonEnabled(false)` — 禁用内置清除按钮
- 创建 `m_matchCountLabel`，默认 `hide()`
- 创建 `m_clearButton`（`DIconButton(QStyle::SP_LineEditClearButton)`，16x16，NoFocus），默认 `hide()`
- 创建容器 `QWidget`，内部 `QHBoxLayout`（contentsMargins=0, spacing=0）依次 `addWidget(label)` + `addSpacing(6)` + `addWidget(clearButton)`
- `setRightWidgets({container})`
- 清除按钮 clicked → `lineEdit()->clear()`；可见性由 `handleTextChanged` 跟随文本非空控制

#### FindBar（被动更新）

文件：`src/controls/findbar.h` / `src/controls/findbar.cpp`

新增公共槽：
- `void slotUpdateMatchCount(int current, int total)` → `m_editLine->setMatchCount(current, total)`

`findCancel`（findbar.cpp:126）在 `hide()` 前调用 `m_editLine->setMatchCount(0, 0)` 清空，避免下次打开残留。

#### ReplaceBar（被动更新，仅左侧）

文件：`src/controls/replacebar.h` / `src/controls/replacebar.cpp`

新增公共槽：
- `void slotUpdateMatchCount(int current, int total)` → 仅 `m_replaceLine->setMatchCount(current, total)`，`m_withLine` 不调用

`replaceClose` 在 hide 前调用 `m_replaceLine->setMatchCount(0, 0)`。

### 4.3 Window 层（per-wrapper 连线）

文件：`src/widgets/window.cpp`

代码库中无 `EditArea` 类，Window 直接对每个 `EditWrapper::textEditor()` 的信号做连线/断开（window.cpp:785-790 连、895-896 断、1108-1116 连、1234-1235 断、4344 断）。新增信号遵循同一模式。

在每处 wrapper 创建/绑定信号的代码块中，紧邻现有 `connect(wrapper->textEditor(), &TextEdit::... ...)` 之后新增：

```cpp
connect(wrapper->textEditor(), &TextEdit::findMatchCountChanged,
        m_findBar, &FindBar::slotUpdateMatchCount, Qt::QueuedConnection);
connect(wrapper->textEditor(), &TextEdit::findMatchCountChanged,
        m_replaceBar, &ReplaceBar::slotUpdateMatchCount, Qt::QueuedConnection);
```

**disconnect 说明**（修正原稿误导性表述）：
- window.cpp:896 形如 `disconnect(wrapper->textEditor(), &TextEdit::textChanged, nullptr, nullptr)`，只断开 `textChanged`，**不会**自动断开新增的 `findMatchCountChanged`
- 新增信号靠后续 `wrapper->deleteLater()`（window.cpp:895/1234/4346）引发的 QObject 析构自动断开——发送端销毁后信号自然失效
- window.cpp:4344 的 `disconnect(wrapper->textEditor())` 是关闭窗口时的批量断开，已覆盖
- 结论：**新增信号无需在任何 disconnect 处显式处理**，依赖 QObject 生命周期管理即可

Tab 切换时清理显示：在 `Window::handleCurrentChanged`（由 window.cpp:598 `connect(m_tabbar, &DTabBar::currentChanged, this, &Window::handleCurrentChanged)` 触发）中调用：

```cpp
m_findBar->slotUpdateMatchCount(0, 0);
m_replaceBar->slotUpdateMatchCount(0, 0);
```

理由：切到新 Tab 时旧 wrapper 信号断开，若新 Tab 未搜索过，两 Bar 应清空残留的上一 Tab 数字（total=0 触发 label hide）。

## 5. 边界情况

| 场景 | 处理 |
|------|------|
| 空关键字 | `total==0` → label 隐藏，与 `setMismatchAlert(false)` 一致 |
| 无匹配 | `total==0` → label 隐藏，输入框红色警示保留 |
| 有匹配但当前光标不在任何匹配项 | `current=0, total>0` → 显示 `第0/N项` |
| 回环搜索（`updateCursorKeywordSelection` 从 Start 重搜，dtextedit.cpp:2355） | 重搜后光标落在某个匹配项上，索引计算照常 |
| 替换操作改变文档 | 替换命令修改文档文本 → 触发 `QPlainTextEdit::textChanged` → 自动 `invalidateMatchCountCache`，下次搜索重扫 |
| Tab 切换 | `Window::handleCurrentChanged` 中调用两 Bar 的 `slotUpdateMatchCount(0,0)` 清空显示 |
| FindBar/ReplaceBar 关闭 | hide 前调 `setMatchCount(0,0)` 清空 |
| 两个 Bar 同时接同一信号 | 只有当前可见 Bar 实际刷新；隐藏 Bar 更新不可见，无副作用 |

## 6. 性能

- 全文档扫描仅在缓存失效时执行一次；后续 Next/Prev 导航只做二分查找，O(log N)
- 缓存失效触发：关键字/大小写变化（在 `updateMatchCount` 内判断）、文档内容变化（`QPlainTextEdit::textChanged` → `invalidateMatchCountCache`，单一入口覆盖替换命令等所有文本修改场景）
- 超大文件首次扫描若卡顿，后续可优化为异步（本设计不含）

## 7. 测试

沿用 `tests/` 现有 QTest 约定（测试用 `-fno-access-control` 编译，可访问私有成员，但优先通过 public 接口测试）。

`tests/src/editor/ut_textedit.cpp` — 纯函数直接测试（推荐，无编译 hack 依赖）：
- `scanAllMatchPositions` 全文档计数：构造 `"hello world\nhello world"`，扫描 `"world"` → 返回列表 size==2，且位置升序
- `scanAllMatchPositions` 空关键字：返回空列表
- `scanAllMatchPositions` 大小写：`"Hello"` + `Qt::CaseSensitive` 在 `"hello world\nHello world"` 中只匹配 1 个；`Qt::CaseInsensitive` 匹配 2 个
- `scanAllMatchPositions` 无匹配：返回空列表
- `scanAllMatchPositions` 重叠不匹配：`"aaa"` 在 `"aaaa"` 中按 Qt `QTextDocument::find` 语义返回预期数量（不重叠）
- `findCurrentMatchIndex` 命中：手动设置 `m_findHighlightSelection.cursor` 到第 k 个匹配起始位置，`m_allMatchPositions` 预置后调用 → 返回 k
- `findCurrentMatchIndex` 未命中：cursor 位置不在列表中 → 返回 0
- `findCurrentMatchIndex` 空缓存：`m_allMatchPositions` 为空 → 返回 0

`tests/src/editor/ut_textedit.cpp` — 编排函数（依赖 `-fno-access-control`，用 QSignalSpy）：
- `updateMatchCount` 空关键字：`emit findMatchCountChanged(0, 0)`
- `updateMatchCount` 缓存命中：相同 keyword + caseFlag 二次调用，`m_allMatchPositions` 不变（可用 QSpy 或对比调用前后 size）
- `updateMatchCount` 缓存失效：触发 `textChanged` 后 `m_allMatchPositions` 清空
- `updateMatchCount` emit 验证：构造 `"hello\nhello\nhello"`，搜索 `"hello"` → QSignalSpy 捕获 `findMatchCountChanged(current, 3)`
- `updateMatchCount` 无双重 emit：调用 `highlightKeyword` 后 QSignalSpy 只捕获 **1 次** 信号（验证不在 `highlightKeyword` 重复调用的回归保护）
- `invalidateMatchCountCache`：调用后 `m_countedKeyword` 为空、`m_allMatchPositions` 为空

`tests/src/controls/ut_linebar.cpp`：
- `setMatchCount(5,10)` 后 `m_matchCountLabel` 文本为 "第5/10项" 且可见（`isVisible()` 为 true）
- `setMatchCount(0,0)` 后 `m_matchCountLabel` 隐藏（`isVisible()` 为 false）
- 计数 label 与自绘清除按钮在同一容器中，label 在清除按钮左侧（用 `layout->indexOf()` 断言顺序：label index < button index）；内置清除按钮已禁用（`isClearButtonEnabled()==false`）；6px 间距由容器内 spacer 保证（headless 环境无法验证像素）

`tests/src/controls/ut_findbar.cpp` / `ut_replacebar.cpp`：
- `slotUpdateMatchCount` 正确转发到内部 LineBar：调用后 LineBar 的 label 文本正确
- ReplaceBar 只更新 `m_replaceLine`：调用 `slotUpdateMatchCount(5,10)` 后，`m_replaceLine` label 为 "第5/10项"，`m_withLine` label 始终隐藏
- FindBar/ReplaceBar 关闭（`findCancel`/`replaceClose`）后 LineBar label 隐藏

## 8. 风险点

- **方案 A 左右顺序**：计数 label 是否真的排在清除按钮左侧依赖 DTK 版本行为。实现后需实测；若顺序相反，先尝试 stylesheet 补偿，仍不行再评估方案 B。
- **大文档性能**：首次全文档扫描可能卡顿；缓存机制保证只扫一次，可接受。超大文件场景后续可异步化。
- **信号 disconnect 正确性**：新增信号靠 wrapper 的 QObject 析构（`deleteLater()`）自动断开；已确认 window.cpp:4344 的 `disconnect(wrapper->textEditor())` 在窗口关闭时批量断开。风险点在于若某处销毁 wrapper 但未调 `deleteLater`，信号会悬挂——实现时需复核所有 wrapper 销毁路径。

## 9. 涉及改动文件清单

- `src/editor/dtextedit.h` — 新增成员；信号 `findMatchCountChanged`；public 纯函数 `scanAllMatchPositions` / `findCurrentMatchIndex`；private 编排 `updateMatchCount` / `invalidateMatchCountCache`
- `src/editor/dtextedit.cpp` — 实现上述；仅在 `updateCursorKeywordSelection` 末尾调用 `updateMatchCount`（**不在** `highlightKeyword` 中调用，避免双重 emit）；构造函数连接 `textChanged` → `invalidateMatchCountCache`（单一缓存失效入口）
- `src/controls/linebar.h` — 新增 `m_matchCountLabel`、`setMatchCount`
- `src/controls/linebar.cpp` — 构造函数加 label 与 QWidgetAction；实现 `setMatchCount`
- `src/controls/findbar.h` — 新增 `slotUpdateMatchCount`
- `src/controls/findbar.cpp` — 实现槽；`findCancel` 中清空
- `src/controls/replacebar.h` — 新增 `slotUpdateMatchCount`
- `src/controls/replacebar.cpp` — 实现槽（仅作用于 `m_replaceLine`）；`replaceClose` 中清空
- `src/widgets/window.cpp` — per-wrapper connect 块新增两行；`Window::handleCurrentChanged` 中清空两 Bar 显示（disconnect 依赖 QObject 析构，无需显式处理）
- `tests/src/editor/ut_textedit.cpp` — `scanAllMatchPositions`/`findCurrentMatchIndex` 纯函数用例（public 接口）；`updateMatchCount`/`invalidateMatchCountCache` 编排用例（含无双重 emit 回归断言）
- `tests/src/controls/ut_linebar.cpp` — `setMatchCount` 用例（含 styleSheet 字符串断言）
- `tests/src/controls/ut_findbar.cpp` — 槽转发用例
- `tests/src/controls/ut_replacebar.cpp` — 槽转发用例（仅 `m_replaceLine`）
