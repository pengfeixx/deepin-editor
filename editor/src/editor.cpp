/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "editor.h"
#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

Editor::Editor(QWidget *parent) : QWidget(parent)
{
    // Init.
    m_autoSaveInternal = 1000;
    m_saveFinish = true;

    // Init layout and widgets.
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    textEditor = new TextEditor;

    m_layout->addWidget(textEditor->lineNumberArea);
    m_layout->addWidget(textEditor);
}

void Editor::loadFile(QString filepath)
{
    QFile file(filepath);
    if (file.open(QFile::ReadOnly)) {
        auto fileContent = file.readAll();
        m_fileEncode = Utils::getFileEncode(fileContent, filepath);

        qDebug() << QString("Detect file %1 with encoding: %2").arg(filepath).arg(QString(m_fileEncode));

        QTextStream stream(&fileContent);
        stream.setCodec(m_fileEncode);
        textEditor->setPlainText(stream.readAll());

        updatePath(filepath);

        textEditor->loadHighlighter();
    }

    file.close();
}

void Editor::saveFile(QString encode, QString newline)
{
    bool fileCreateFailed = false;
    if (!Utils::fileExists(textEditor->filepath)) {
        QString directory = QFileInfo(textEditor->filepath).dir().absolutePath();

        // Create file if filepath is not exists.
        if (Utils::fileIsWritable(directory)) {
            QDir().mkpath(directory);
            if (QFile(textEditor->filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(textEditor->filepath);
            }
        } else {
            // Make flag fileCreateFailed with 'true' if no permission to create.
            fileCreateFailed = true;
        }
    }

    // Try save file if file has permission.
    if (Utils::fileIsWritable(textEditor->filepath) && !fileCreateFailed) {
        QFile file(textEditor->filepath);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Can't write file: " << textEditor->filepath;

            return;
        }

        QRegularExpression newlineRegex("\r?\n|\r");
        QString fileNewline;
        if (newline == "Window") {
            fileNewline = "\r\n";
        } else if (newline == "Linux") {
            fileNewline = "\n";
        } else {
            fileNewline = "\r";
        }

        QTextStream out(&file);

        out.setCodec(encode.toUtf8().data());
        // NOTE: Muse call 'setGenerateByteOrderMark' to insert the BOM (Byte Order Mark) before any data has been written to file.
        // Otherwise, can't save file with given encoding.
        out.setGenerateByteOrderMark(true);
        out << textEditor->toPlainText().replace(newlineRegex, fileNewline);

        qDebug() << "saved: " << textEditor->filepath << encode;

        file.close();
    }
}

void Editor::saveFile()
{
    saveFile(m_fileEncode, "Window");
}

void Editor::updatePath(QString file)
{
    textEditor->filepath = file;
}
