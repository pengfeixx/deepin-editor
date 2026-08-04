// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "linebar.h"
#include "../common/utils.h"

#include <DGuiApplicationHelper>
#include <QWidgetAction>

#include <QDebug>

// 不同布局模式(紧凑)
const int s_nLineBarHeight = 36;
const int s_nLineBarHeightCompact = 24;

LineBar::LineBar(DLineEdit *parent)
    : DLineEdit(parent)
{
    qDebug() << "LineBar constructor start";
    // 不使用 DLineEdit 的内置清除按钮（setClearButtonEnabled）。
    // 原因：Qt6 下 QLineEdit::addAction(TrailingPosition) 和 DLineEdit::setRightWidgets
    // 都把自定义 widget 放在内置清除按钮的右侧，无法让计数 label 处于清除按钮左侧。
    // 改为自绘清除按钮，与计数 label 放入同一容器 [label][6px spacer][清除按钮]，
    // 通过 QLineEdit::addAction(TrailingPosition) 添加——容器会覆盖在输入框内部右侧
    // （和原内置清除按钮位置一致），不压缩文本区域。
    setClearButtonEnabled(false);

    m_matchCountLabel = new QLabel();
    m_matchCountLabel->hide();

    m_clearButton = new DIconButton(QStyle::SP_LineEditClearButton);
    m_clearButton->setFixedSize(16, 16);
    m_clearButton->setIconSize(QSize(16, 16));
    m_clearButton->setFocusPolicy(Qt::NoFocus);
    m_clearButton->hide();

    // 容器：label 在最左，6px 间距，清除按钮在最右
    QWidget *rightContainer = new QWidget(this);
    QHBoxLayout *rightLayout = new QHBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_matchCountLabel);
    rightLayout->addSpacing(6);
    rightLayout->addWidget(m_clearButton);

    // 用 QLineEdit::addAction 覆盖在输入框内部右侧（非 setRightWidgets——后者会压缩文本区域）
    QWidgetAction *rightAction = new QWidgetAction(this);
    rightAction->setDefaultWidget(rightContainer);
    lineEdit()->addAction(rightAction, QLineEdit::TrailingPosition);

    m_autoSaveInternal = 50;
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    qDebug() << "Auto-save timer initialized with interval:" << m_autoSaveInternal << "ms";

    connect(m_autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);
    connect(this, &DLineEdit::textEdited, this, &LineBar::sendText, Qt::QueuedConnection);
    connect(this, &DLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
    qDebug() << "Signal connections established";

    // 自绘清除按钮：点击清空文本；可见性跟随文本是否为空
    connect(m_clearButton, &DIconButton::clicked, this, [this]() {
        qDebug() << "Clear button clicked, clearing text";
        lineEdit()->clear();
    });

#ifdef DTKWIDGET_CLASS_DSizeMode
    setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_nLineBarHeightCompact : s_nLineBarHeight);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [this](){
        setFixedHeight(DGuiApplicationHelper::isCompactMode() ? s_nLineBarHeightCompact : s_nLineBarHeight);
    });
#endif
}

void LineBar::handleTextChangeTimer()
{
    qDebug() << "Text change timer triggered, emitting contentChanged";
    // Emit contentChanged signal.
    contentChanged();
}

void LineBar::handleTextChanged(const QString &str)
{
    // Stop timer if new character is typed, avoid unused timer run.
    if (m_autoSaveTimer->isActive()) {
        qDebug() << "Restarting text change timer";
        m_autoSaveTimer->stop();
    }
    if(str.isEmpty()) {
        qDebug() << "Text cleared, disabling alert";
        setAlert(false);
        m_clearButton->hide();
    } else {
        m_clearButton->show();
    }
    // Start new timer.
    m_autoSaveTimer->start(m_autoSaveInternal);
    qDebug() << "Text changed, length:" << str.length();
}

void LineBar::sendText(QString t)
{
    emit signal_sentText(t);
}

void LineBar::focusOutEvent(QFocusEvent *e)
{
    qDebug() << "Focus lost";
    // Emit focus out signal.
    focusOut();

    // Throw event out avoid DLineEdit can't hide cursor after lost focus.
    DLineEdit::focusOutEvent(e);
}

void LineBar::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);
    Qt::KeyboardModifiers modifiers = e->modifiers();
    qDebug() << "Key pressed:" << key << "modifiers:" << modifiers;

    if(modifiers == Qt::ControlModifier && e->text() == "\r"){
       qDebug() << "Ctrl+Enter pressed";
       pressCtrlEnter();
    }else if(modifiers == Qt::AltModifier && e->text() == "\r"){
       qDebug() << "Alt+Enter pressed";
       pressAltEnter();
    }else if(modifiers == Qt::MetaModifier && e->text() == "\r"){
       qDebug() << "Meta+Enter pressed";
       pressMetaEnter();
    }else if(modifiers == Qt::NoModifier && e->text() == "\r"){
       qDebug() << "Enter pressed";
       pressEnter();
    }else {
      // Pass event to DLineEdit continue, otherwise you can't type anything after here. ;)
       DLineEdit::keyPressEvent(e);
    }
}

void LineBar::setMatchCount(int current, int total)
{
    if (total == 0) {
        m_matchCountLabel->hide();
    } else {
        m_matchCountLabel->setText(QString("第%1/%2项").arg(current).arg(total));
        m_matchCountLabel->show();
    }
}
