/*
 * This file is part of the KDE project
 * Copyright (C) 2015 David Edmundson <davidedmundson@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KCOLLAPSIBLEGROUPBOX_H
#define KCOLLAPSIBLEGROUPBOX_H

#include <QWidget>

class KCollapsibleGroupBoxPrivate;

class KCollapsibleGroupBox : public QWidget
{
   Q_OBJECT
   Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
   Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)

public:
   explicit KCollapsibleGroupBox(QWidget *parent = nullptr);
   ~KCollapsibleGroupBox() override;

   void setTitle(const QString &title);

   QString title() const;

   void setExpanded(bool expanded);

   bool isExpanded() const;

   QSize sizeHint() const override;
   QSize minimumSizeHint() const override;

public Q_SLOTS:
   void toggle();

   void expand();

   void collapse();

Q_SIGNALS:
   void titleChanged();
   void expandedChanged();

protected:
   void paintEvent(QPaintEvent*) override;

   bool event(QEvent*) override;
   void mousePressEvent(QMouseEvent*) override;
   void mouseMoveEvent(QMouseEvent*) override;
   void leaveEvent(QEvent*) override;
   void keyPressEvent(QKeyEvent*) override;
   void resizeEvent(QResizeEvent*) override;

private Q_SLOTS:
   void overrideFocusPolicyOf(QWidget *widget);

private:
   KCollapsibleGroupBoxPrivate *const d;

   Q_DISABLE_COPY(KCollapsibleGroupBox)
};

#endif
