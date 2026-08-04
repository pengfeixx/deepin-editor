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

//计数 label 与自绘清除按钮直接 parent 到 lineEdit()（不用容器），
//通过 setGeometry 精确定位：label 在 button 左侧、间距 6px、button 右边缘距输入框右边 6px
TEST_F(test_linebar, m_matchCountLabel_LeftOfClearButton)
{
    LineBar *lineBar = new LineBar();
    // 触发 label 显示 + 初始化坐标
    lineBar->setMatchCount(5, 10);

    // label 和清除按钮的 parent 应为 lineEdit()
    EXPECT_EQ(lineBar->m_matchCountLabel->parentWidget(), lineBar->lineEdit());
    EXPECT_EQ(lineBar->m_clearButton->parentWidget(), lineBar->lineEdit());

    // 坐标断言：label 与 button 间距 6px（用 x + width 计算 label 右边缘外侧，避开 geometry.right() 的含尾像素）
    const QRect labelGeom = lineBar->m_matchCountLabel->geometry();
    const QRect buttonGeom = lineBar->m_clearButton->geometry();
    const int labelRightOuter = labelGeom.x() + labelGeom.width();
    EXPECT_EQ(buttonGeom.x() - labelRightOuter, 6);

    // button 右边缘距 lineEdit 右边缘 6px
    const int buttonRightOuter = buttonGeom.x() + buttonGeom.width();
    EXPECT_EQ(lineBar->lineEdit()->width() - buttonRightOuter, 6);

    // label 应在 button 左侧
    EXPECT_LT(labelRightOuter, buttonGeom.x());

    lineBar->deleteLater();
}

//内置清除按钮已禁用（改用自绘清除按钮以控制顺序）
TEST_F(test_linebar, builtinClearButtonDisabled)
{
    LineBar *lineBar = new LineBar();

    EXPECT_FALSE(lineBar->isClearButtonEnabled());

    lineBar->deleteLater();
}
