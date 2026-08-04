// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_linebar.h"
#include "../../src/controls/linebar.h"
#include <QFocusEvent>
#include <QEvent>
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

//m_matchCountLabel 位于带布局的容器中（容器内 label 右侧有 6px spacer，
//保证 label 右边缘距清除按钮左侧 6px）
TEST_F(test_linebar, m_matchCountLabel_InLayoutContainer)
{
    LineBar *lineBar = new LineBar();

    // label 应被放进一个容器 widget（parent 不为空且该容器有 layout）
    QWidget *container = lineBar->m_matchCountLabel->parentWidget();
    ASSERT_NE(container, nullptr);
    ASSERT_NE(container->layout(), nullptr);

    // 容器布局应为 QHBoxLayout，包含 label 和一个 6px 的 spacer
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(container->layout());
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->spacing(), 0);

    lineBar->deleteLater();
}
