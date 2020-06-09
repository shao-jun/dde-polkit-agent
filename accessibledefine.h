/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     fpc_diesel <fanpengcheng@uniontech.com>
 *
 * Maintainer: fpc_diesel <fanpengcheng@uniontech.com>
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
#ifndef ACCESSIBLEDEFINE_H
#define ACCESSIBLEDEFINE_H

// 为了方便使用,把相关定义独立出来,如有需要,直接包含这个头文件,然后使用SET_*的宏去设置,USE_*宏开启即可
// 注意：对项目中出现的所有的QWidget的派生类都要再启用一次accessiblity，包括qt的原生控件[qt未限制其标记名称为空的情况]
// 注意：使用USE_ACCESSIBLE_BY_OBJECTNAME开启accessiblity的时候，一定要再最这个类用一下USE_ACCESSIBLE，否则标记可能会遗漏

/* 宏参数说明
* classname:类名,例如DLineEdit
* accessiblename:accessible唯一标识,需保证唯一性[getAccessibleName函数处理],优先使用QObject::setAccessibleName值
* accessibletype:即QAccessible::Role,表示标识控件的类型
* classobj:QObject指针
* accessdescription:accessible描述内容,考虑到暂时用不到,目前都默认为空,有需要可自行设计接口
*
* 部分创建宏说明
* FUNC_CREATE:创建构造函数
* FUNC_PRESS:创建Press接口
* FUNC_SHOWMENU:创建右键菜单接口
* FUNC_PRESS_SHOWMENU:上两者的综合
* FUNC_RECT:实现rect接口
* FUNC_TEXT:实现text接口
* FUNC_TEXT_BY_LABEL:实现text接口,使用这个宏，需要被添加的类有text接口，从而得到其Value
* USE_ACCESSIBLE:对传入的类型设置其accessible功能
* USE_ACCESSIBLE_BY_OBJECTNAME:同上,[指定objectname]---适用同一个类，但objectname不同的情况
*
* 设置为指定类型的Accessible控件宏
* SET_BUTTON_ACCESSIBLE_PRESS_SHOWMENU:button类型,添加press和showmenu功能
* SET_BUTTON_ACCESSIBLE_SHOWMENU:button类型,添加showmenu功能
* SET_BUTTON_ACCESSIBLE:button类型,添加press功能
* SET_LABEL_ACCESSIBLE:label类型,用于标签控件
* SET_FORM_ACCESSIBLE:form类型,用于widget控件
* SET_SLIDER_ACCESSIBLE:slider类型,用于滑块控件
* SET_SEPARATOR_ACCESSIBLE:separator类型,用于分隔符控件
*/
#include "accessiblemap.h"

#include <QAccessible>
#include <QAccessibleWidget>
#include <QEvent>
#include <QMap>
#include <QString>
#include <QWidget>
#include <QObject>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QApplication>

#define SEPARATOR "_"

inline QString getAccessibleName(QWidget *w, QAccessible::Role r, const QString &fallback)
{
    const QString lowerFallback = fallback.toLower();
    // 避免重复生成
    static QMap< QObject *, QString > objnameMap;
    if (!objnameMap[w].isEmpty())
        return objnameMap[w];

    QString oldAccessName = w->accessibleName().toLower();
    if(oldAccessName.isEmpty())
        oldAccessName = w->objectName().toLower();
    oldAccessName.replace(SEPARATOR, "");

    // 按照类型添加固定前缀
    QMetaEnum metaEnum = QMetaEnum::fromType<QAccessible::Role>();
    QByteArray prefix = metaEnum.valueToKeys(r);
    switch (r) {
    case QAccessible::Button:       prefix = "Btn";         break;
    case QAccessible::StaticText:   prefix = "Label";       break;
    default:                        break;
    }

    // 再加上标识
    QString accessibleName = QString::fromLatin1(prefix) + SEPARATOR;
    accessibleName += oldAccessName.isEmpty() ? lowerFallback : oldAccessName;

    // 检查名称是否唯一
    if (AccessibleMap::instance()->accessibleMap()[r].contains(accessibleName)) {
        if (!objnameMap.key(accessibleName)) {
            objnameMap.remove(objnameMap.key(accessibleName));
            objnameMap.insert(w, accessibleName);
            return accessibleName;
        }
        // 获取编号，然后+1
        int pos = accessibleName.indexOf(SEPARATOR);
        int id = accessibleName.mid(pos + 1).toInt();

        QString newAccessibleName;
        do {
            // 一直找到一个不重复的名字
            newAccessibleName = accessibleName + SEPARATOR + QString::number(++id);
        } while (AccessibleMap::instance()->accessibleMap()[r].contains(newAccessibleName));
        AccessibleMap::instance()->accessibleMapAppend(r,newAccessibleName);
        objnameMap.insert(w, newAccessibleName);

        return newAccessibleName;
    } else {
        AccessibleMap::instance()->accessibleMapAppend(r,accessibleName);
        objnameMap.insert(w, accessibleName);

        return accessibleName;
    }
}

#define FUNC_CREATE(classname,accessibletype,accessdescription)    Accessible##classname(classname *w) \
        : QAccessibleWidget(w,accessibletype,#classname)\
        , m_w(w)\
        , m_description(accessdescription)\
    {}\


#define FUNC_PRESS(classobj)     QStringList actionNames() const override{\
        if(!classobj->isEnabled())\
            return QStringList();\
        return QStringList() << pressAction();}\
    void doAction(const QString &actionName) override{\
        if(actionName == pressAction())\
        {\
            QPointF localPos = classobj->geometry().center();\
            QMouseEvent event(QEvent::MouseButtonPress,localPos,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);\
            QMouseEvent event2(QEvent::MouseButtonRelease,localPos,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);\
            qApp->sendEvent(classobj,&event);\
            qApp->sendEvent(classobj,&event2);\
        }\
    }\

#define FUNC_SHOWMENU(classobj)     QStringList actionNames() const override{\
        if(!classobj->isEnabled())\
            return QStringList();\
        return QStringList() << showMenuAction();}\
    void doAction(const QString &actionName) override{\
        if(actionName == showMenuAction())\
        {\
            QPointF localPos = classobj->geometry().center();\
            QMouseEvent event(QEvent::MouseButtonPress,localPos,Qt::RightButton,Qt::RightButton,Qt::NoModifier);\
            QMouseEvent event2(QEvent::MouseButtonRelease,localPos,Qt::RightButton,Qt::RightButton,Qt::NoModifier);\
            qApp->sendEvent(classobj,&event);\
            qApp->sendEvent(classobj,&event2);\
        }\
    }\

#define FUNC_PRESS_SHOWMENU(classobj)     QStringList actionNames() const override{\
        if(!classobj->isEnabled())\
            return QStringList();\
        return QStringList() << pressAction() << showMenuAction();}\
    void doAction(const QString &actionName) override{\
        if(actionName == pressAction())\
        {\
            QPointF localPos = classobj->geometry().center();\
            QMouseEvent event(QEvent::MouseButtonPress,localPos,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);\
            QMouseEvent event2(QEvent::MouseButtonRelease,localPos,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);\
            qApp->sendEvent(classobj,&event);\
            qApp->sendEvent(classobj,&event2);\
        }\
        else if(actionName == showMenuAction())\
        {\
            QPointF localPos = classobj->geometry().center();\
            QMouseEvent event(QEvent::MouseButtonPress,localPos,Qt::RightButton,Qt::RightButton,Qt::NoModifier);\
            QMouseEvent event2(QEvent::MouseButtonRelease,localPos,Qt::RightButton,Qt::RightButton,Qt::NoModifier);\
            qApp->sendEvent(classobj,&event);\
            qApp->sendEvent(classobj,&event2);\
        }\
    }\

#define FUNC_RECT(classobj) QRect rect() const override{\
        if (!classobj->isVisible())\
            return QRect();\
        QPoint pos = classobj->mapToGlobal(QPoint(0, 0));\
        return QRect(pos.x(), pos.y(), classobj->width(), classobj->height());\
    }\

#define FUNC_TEXT(classname,accessiblename) QString Accessible##classname::text(QAccessible::Text t) const{\
        switch (t) {\
        case QAccessible::Name:\
            return getAccessibleName(m_w, this->role(), accessiblename);\
        case QAccessible::Description:\
            return m_description;\
        default:\
            return QString();\
        }\
    }\

    // Label控件特有功能
#define FUNC_TEXT_(classname) QString Accessible##classname::text(int startOffset, int endOffset) const{\
        Q_UNUSED(startOffset)\
        Q_UNUSED(endOffset)\
        return m_w->text();\
    }\



#define FUNC_TEXT_BY_LABEL(classname,accessiblename) QString Accessible##classname::text(QAccessible::Text t) const{\
        switch (t) {\
        case QAccessible::Name:\
            return getAccessibleName(m_w, this->role(), accessiblename);\
        case QAccessible::Description:\
            return m_description;\
        case QAccessible::Value:\
            return m_w->text();\
        default:\
            return QString();\
        }\
    }\

#define USE_ACCESSIBLE(classnamestring,classname)    if (classnamestring == QLatin1String(#classname) && object && object->isWidgetType())\
    {\
        interface = new Accessible##classname(static_cast<classname *>(object));\
    }\

#define USE_ACCESSIBLE_BY_OBJECTNAME(classnamestring,classname,objectname)    if (classnamestring == QLatin1String(#classname) && object && (object->objectName() == objectname) && object->isWidgetType())\
    {\
        interface = new Accessible##classname(static_cast<classname *>(object));\
    }\

#define SET_ACCESSIBLE_WITH_PRESS_SHOWMENU_DESCRIPTION(classname,aaccessibletype,accessdescription)  class Accessible##classname : public QAccessibleWidget\
    {\
    public:\
        FUNC_CREATE(classname,aaccessibletype,accessdescription)\
        QString text(QAccessible::Text t) const override;\
    private:\
    classname *m_w;\
    QString m_description;\
    FUNC_PRESS_SHOWMENU(m_w)\
    };\

#define SET_ACCESSIBLE_WITH_DESCRIPTION(classname,aaccessibletype,accessdescription)  class Accessible##classname : public QAccessibleWidget\
    {\
    public:\
        FUNC_CREATE(classname,aaccessibletype,accessdescription)\
        QString text(QAccessible::Text t) const override;\
    private:\
    classname *m_w;\
    QString m_description;\
    FUNC_RECT(m_w)\
    };\

#define SET_LABEL_ACCESSIBLE_WITH_DESCRIPTION(classname,accessiblename,accessdescription)  class Accessible##classname : public QAccessibleWidget, public QAccessibleTextInterface\
    {\
    public:\
        FUNC_CREATE(classname,QAccessible::StaticText,accessdescription)\
        QString text(QAccessible::Text t) const override;\
        void *interface_cast(QAccessible::InterfaceType t) override{\
                switch (t) {\
                case QAccessible::ActionInterface:\
                    return static_cast<QAccessibleActionInterface*>(this);\
                case QAccessible::TextInterface:\
                    return static_cast<QAccessibleTextInterface*>(this);\
                default:\
                    return nullptr;\
                }\
            }\
        QString text(int startOffset, int endOffset) const override;\
        void selection(int selectionIndex, int *startOffset, int *endOffset) const override \
        {Q_UNUSED(selectionIndex) Q_UNUSED(startOffset) Q_UNUSED(endOffset)}\
        int selectionCount() const override { return 0; }\
        void addSelection(int startOffset, int endOffset) override \
        {Q_UNUSED(startOffset) Q_UNUSED(endOffset)}\
        void removeSelection(int selectionIndex) override \
        {Q_UNUSED(selectionIndex)}\
        void setSelection(int selectionIndex, int startOffset, int endOffset) override \
        {Q_UNUSED(selectionIndex) Q_UNUSED(startOffset) Q_UNUSED(endOffset)}\
        int cursorPosition() const override { return 0; }\
        void setCursorPosition(int position) override \
        {Q_UNUSED(position)}\
        int characterCount() const override { return 0; }\
        QRect characterRect(int offset) const override \
        { Q_UNUSED(offset) return QRect(); }\
        int offsetAtPoint(const QPoint &point) const override \
        { Q_UNUSED(point) return 0; }\
        void scrollToSubstring(int startIndex, int endIndex) override \
        {Q_UNUSED(startIndex) Q_UNUSED(endIndex)}\
        QString attributes(int offset, int *startOffset, int *endOffset) const override \
        { Q_UNUSED(offset) Q_UNUSED(startOffset)  Q_UNUSED(endOffset) return QString(); }\
    private:\
    classname *m_w;\
    QString m_description;\
    };\
    FUNC_TEXT(classname,accessiblename)\
    FUNC_TEXT_(classname)\

/*******************************************简化使用*******************************************/
#define SET_BUTTON_ACCESSIBLE(classname,accessiblename)                         SET_ACCESSIBLE_WITH_PRESS_SHOWMENU_DESCRIPTION(classname,QAccessible::Button,"")\
    FUNC_TEXT(classname,accessiblename)

#define SET_MENU_ACCESSIBLE(classname,accessiblename)                           SET_ACCESSIBLE_WITH_PRESS_SHOWMENU_DESCRIPTION(classname,QAccessible::PopupMenu,"")\
    FUNC_TEXT(classname,accessiblename)

#define SET_LABEL_ACCESSIBLE(classname,accessiblename)                          SET_LABEL_ACCESSIBLE_WITH_DESCRIPTION(classname,accessiblename,"")

#define SET_WIDGET_ACCESSIBLE(classname,aaccessibletype,accessiblename)         SET_ACCESSIBLE_WITH_DESCRIPTION(classname,aaccessibletype,"");\
    FUNC_TEXT(classname,accessiblename)

/************************************************************************************************/

#endif // ACCESSIBLEDEFINE_H