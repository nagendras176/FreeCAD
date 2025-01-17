/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <QEvent>

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>

#include "ui_TaskSketcherGeneral.h"
#include "TaskSketcherGeneral.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

SketcherGeneralWidget::SketcherGeneralWidget(QWidget *parent)
  : QWidget(parent), ui(new Ui_TaskSketcherGeneral)
{
    ui->setupUi(this);
    ui->renderingOrder->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    // connecting the needed signals
    connect(ui->checkBoxAutoconstraints, &QCheckBox::toggled,
            this, &SketcherGeneralWidget::emitToggleAutoconstraints);
    connect(ui->checkBoxRedundantAutoconstraints, &QCheckBox::toggled,
            this, &SketcherGeneralWidget::emitToggleAvoidRedundant);
    ui->renderingOrder->installEventFilter(this);
}

SketcherGeneralWidget::~SketcherGeneralWidget()
{
}

bool SketcherGeneralWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->renderingOrder && event->type() == QEvent::ChildRemoved) {
        Q_EMIT emitRenderOrderChanged();
    }
    return false;
}

void SketcherGeneralWidget::saveSettings()
{
    ui->checkBoxAutoconstraints->onSave();
    ui->checkBoxRedundantAutoconstraints->onSave();

    saveOrderingOrder();
}

void SketcherGeneralWidget::saveOrderingOrder()
{
    int topid = ui->renderingOrder->item(0)->data(Qt::UserRole).toInt();
    int midid = ui->renderingOrder->item(1)->data(Qt::UserRole).toInt();
    int lowid = ui->renderingOrder->item(2)->data(Qt::UserRole).toInt();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetInt("TopRenderGeometryId",topid);
    hGrp->SetInt("MidRenderGeometryId",midid);
    hGrp->SetInt("LowRenderGeometryId",lowid);
}

void SketcherGeneralWidget::loadSettings()
{
    ui->checkBoxAutoconstraints->onRestore();
    ui->checkBoxRedundantAutoconstraints->onRestore();

    loadOrderingOrder();
}

void SketcherGeneralWidget::loadOrderingOrder()
{
    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    // 1->Normal Geometry, 2->Construction, 3->External
    int topid = hGrpp->GetInt("TopRenderGeometryId",1);
    int midid = hGrpp->GetInt("MidRenderGeometryId",2);
    int lowid = hGrpp->GetInt("LowRenderGeometryId",3);

    {
        QSignalBlocker block(ui->renderingOrder);
        ui->renderingOrder->clear();

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(topid));
        newItem->setText( topid==1?tr("Normal Geometry"):topid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(0,newItem);

        newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(midid));
        newItem->setText(midid==1?tr("Normal Geometry"):midid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(1,newItem);

        newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, QVariant(lowid));
        newItem->setText(lowid==1?tr("Normal Geometry"):lowid==2?tr("Construction Geometry"):tr("External Geometry"));
        ui->renderingOrder->insertItem(2,newItem);
    }
}

void SketcherGeneralWidget::checkAutoconstraints(bool on)
{
    ui->checkBoxAutoconstraints->setChecked(on);
}

void SketcherGeneralWidget::checkAvoidRedundant(bool on)
{
    ui->checkBoxRedundantAutoconstraints->setChecked(on);
}

void SketcherGeneralWidget::enableAvoidRedundant(bool on)
{
    ui->checkBoxRedundantAutoconstraints->setEnabled(on);
}

void SketcherGeneralWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

// ----------------------------------------------------------------------------

TaskSketcherGeneral::TaskSketcherGeneral(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Edit controls"),true, nullptr)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    widget = new SketcherGeneralWidget(this);
    this->groupLayout()->addWidget(widget);

    {
        //Blocker probably not needed as signals aren't connected yet
        QSignalBlocker block(widget);
        //Load default settings to get ordering order & avoid redundant values
        widget->loadSettings();
        widget->checkAutoconstraints(sketchView->Autoconstraints.getValue());
        widget->checkAvoidRedundant(sketchView->AvoidRedundant.getValue());
        widget->enableAvoidRedundant(sketchView->Autoconstraints.getValue());
    }

    // connecting the needed signals
    QObject::connect(
        widget, &SketcherGeneralWidget::emitToggleAutoconstraints,
        this  , &TaskSketcherGeneral::onToggleAutoconstraints
    );

    QObject::connect(
        widget, &SketcherGeneralWidget::emitToggleAvoidRedundant,
        this  , &TaskSketcherGeneral::onToggleAvoidRedundant
    );

    QObject::connect(
        widget, &SketcherGeneralWidget::emitRenderOrderChanged,
        this  , &TaskSketcherGeneral::onRenderOrderChanged
    );

    Gui::Selection().Attach(this);

    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(boost::bind
        (&TaskSketcherGeneral::onChangedSketchView, this, bp::_1, bp::_2));
}

TaskSketcherGeneral::~TaskSketcherGeneral()
{
    Gui::Selection().Detach(this);
}

void TaskSketcherGeneral::onChangedSketchView(const Gui::ViewProvider& vp,
                                              const App::Property& prop)
{
    if (sketchView == &vp) {
        if (&sketchView->Autoconstraints == &prop) {
            QSignalBlocker block(widget);
            widget->checkAutoconstraints(sketchView->Autoconstraints.getValue());
            widget->enableAvoidRedundant(sketchView->Autoconstraints.getValue());
        }
        else if (&sketchView->AvoidRedundant == &prop) {
            QSignalBlocker block(widget);
            widget->checkAvoidRedundant(sketchView->AvoidRedundant.getValue());
        }
    }
}

void TaskSketcherGeneral::onToggleAutoconstraints(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->Autoconstraints.setValue(on);
    widget->enableAvoidRedundant(on);
}

void TaskSketcherGeneral::onToggleAvoidRedundant(bool on)
{
    Base::ConnectionBlocker block(changedSketchView);
    sketchView->AvoidRedundant.setValue(on);
}

/// @cond DOXERR
void TaskSketcherGeneral::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                   Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(Reason);
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR

void TaskSketcherGeneral::onRenderOrderChanged()
{
    widget->saveOrderingOrder();
    sketchView->updateColor();
}

#include "moc_TaskSketcherGeneral.cpp"
