// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "linebar.h"
#include "../common/utils.h"

#include <DGuiApplicationHelper>

#include <QDebug>

// 不同布局模式(紧凑)
const int s_nLineBarHeight = 36;
const int s_nLineBarHeightCompact = 24;

LineBar::LineBar(DLineEdit *parent)
    : DLineEdit(parent)
{
    qDebug() << "LineBar constructor start";
    // Init.
    setClearButtonEnabled(true);

    m_matchCountLabel = new QLabel();
    m_matchCountLabel->hide();

    // 用容器 widget + 布局实现 label 与内置清除按钮的 6px 间距。
    // QLineEdit::addAction 把 action widget 紧贴放置（无 action 间距 API，stylesheet margin
    // 在 QLineEdit 内部布局中也不可靠），因此在容器内部 label 右侧加 6px spacer：
    // 容器右边缘紧贴清除按钮，label 右边缘距清除按钮左侧 6px。
    QWidget *matchCountContainer = new QWidget(this);
    QHBoxLayout *matchCountLayout = new QHBoxLayout(matchCountContainer);
    matchCountLayout->setContentsMargins(0, 0, 0, 0);
    matchCountLayout->setSpacing(0);
    matchCountLayout->addWidget(m_matchCountLabel);
    matchCountLayout->addSpacing(6);

    QWidgetAction *matchCountAction = new QWidgetAction(this);
    matchCountAction->setDefaultWidget(matchCountContainer);
    lineEdit()->addAction(matchCountAction, QLineEdit::TrailingPosition);

    m_autoSaveInternal = 50;
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    qDebug() << "Auto-save timer initialized with interval:" << m_autoSaveInternal << "ms";

    connect(m_autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);
    connect(this, &DLineEdit::textEdited, this, &LineBar::sendText, Qt::QueuedConnection);
    connect(this, &DLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
    qDebug() << "Signal connections established";
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
