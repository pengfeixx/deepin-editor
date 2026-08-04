// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_linebar.h"
#include "../../src/controls/linebar.h"
#include <QFocusEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
DGUI_USE_NAMESPACE
DGUI_USE_NAMESPACE

test_linebar::test_linebar()
{

}

TEST_F(test_linebar, LineBar)
{
    LineBar lineBar(nullptr);
    
}

//public slots:
//    void handleTextChangeTimer();
TEST_F(test_linebar, handleTextChangeTimer)
{
    LineBar *lineBar = new LineBar();
    lineBar->handleTextChangeTimer();


     EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
    
}

//    void handleTextChanged();
TEST_F(test_linebar, handleTextChanged)
{
    LineBar *lineBar = new LineBar();
    lineBar->m_autoSaveTimer->start();

    lineBar->handleTextChanged();

    lineBar->m_autoSaveTimer->stop();

    EXPECT_EQ(lineBar->m_autoSaveTimer->isActive(),false);
    EXPECT_NE(lineBar,nullptr);

   lineBar->deleteLater();
}

//    void sendText(QString t);
TEST_F(test_linebar, sendText)
{
    LineBar *lineBar = new LineBar();
    lineBar->sendText("aa");
    
    EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
}

//protected:
//    virtual void focusOutEvent(QFocusEvent *e);
TEST_F(test_linebar, focusOutEvent)
{
    LineBar *lineBar = new LineBar();
    QFocusEvent *e = new QFocusEvent(QEvent::FocusIn);
    lineBar->focusOutEvent(e);


    EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
    delete e;e=nullptr;
    
}

//    virtual void keyPressEvent(QKeyEvent *e);
TEST_F(test_linebar, keyPressEvent)
{
    LineBar *lineBar = new LineBar();
    Qt::KeyboardModifier modefiers[4] = {Qt::ControlModifier,Qt::AltModifier,Qt::MetaModifier,Qt::NoModifier};

    EXPECT_NE(lineBar,nullptr);

    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,1,modefiers[0],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;


    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[1],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;

    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[2],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;

    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[3],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;


    lineBar->deleteLater();
}

// Constructor lambda connected to DGuiApplicationHelper::sizeModeChanged
TEST_F(test_linebar, ConstructorSizeModeLambda)
{
    LineBar *lineBar = new LineBar();
    auto helper = DGuiApplicationHelper::instance();
    auto origMode = helper->sizeMode();

    EXPECT_NO_FATAL_FAILURE(helper->setSizeMode(origMode == DGuiApplicationHelper::NormalMode
                                                    ? DGuiApplicationHelper::CompactMode
                                                    : DGuiApplicationHelper::NormalMode));
    helper->setSizeMode(origMode); // restore

    EXPECT_NE(lineBar, nullptr);
    lineBar->deleteLater();
}

//setMatchCount 显示文本和可见性
TEST_F(test_linebar, setMatchCount_Display)
{
    LineBar *lineBar = new LineBar();
    lineBar->m_matchCountLabel->show();

    lineBar->setMatchCount(5, 10);

    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第5/10项");
    EXPECT_FALSE(lineBar->m_matchCountLabel->isHidden());

    lineBar->deleteLater();
}

//setMatchCount total==0 隐藏
TEST_F(test_linebar, setMatchCount_HideOnZero)
{
    LineBar *lineBar = new LineBar();
    lineBar->m_matchCountLabel->show();

    lineBar->setMatchCount(0, 0);

    EXPECT_TRUE(lineBar->m_matchCountLabel->isHidden());

    lineBar->deleteLater();
}

//setMatchCount 0/N 场景
TEST_F(test_linebar, setMatchCount_ZeroCurrent)
{
    LineBar *lineBar = new LineBar();

    lineBar->setMatchCount(0, 5);

    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第0/5项");
    EXPECT_FALSE(lineBar->m_matchCountLabel->isHidden());

    lineBar->deleteLater();
}

//计数 label 与自绘清除按钮在同一个 rightWidget 容器中，且 label 在清除按钮左侧（顺序对），
//内部用 6px spacer 保证 label 右边缘距清除按钮左侧 6px
TEST_F(test_linebar, m_matchCountLabel_LeftOfClearButton)
{
    LineBar *lineBar = new LineBar();

    // label 和清除按钮应有同一个容器 parent
    QWidget *labelContainer = lineBar->m_matchCountLabel->parentWidget();
    QWidget *buttonContainer = lineBar->m_clearButton->parentWidget();
    ASSERT_NE(labelContainer, nullptr);
    EXPECT_EQ(labelContainer, buttonContainer);

    // 容器布局为 QHBoxLayout，spacing==0（间距由 addSpacing 保证）
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(labelContainer->layout());
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->spacing(), 0);

    // 顺序断言：label 在清除按钮之前（index 更小 → 视觉上更靠左）
    int labelIndex = layout->indexOf(lineBar->m_matchCountLabel);
    int buttonIndex = layout->indexOf(lineBar->m_clearButton);
    ASSERT_GE(labelIndex, 0);
    ASSERT_GE(buttonIndex, 0);
    EXPECT_LT(labelIndex, buttonIndex);

    lineBar->deleteLater();
}

//内置清除按钮已禁用（改用自绘清除按钮以控制顺序）
TEST_F(test_linebar, builtinClearButtonDisabled)
{
    LineBar *lineBar = new LineBar();

    EXPECT_FALSE(lineBar->isClearButtonEnabled());

    lineBar->deleteLater();
}
