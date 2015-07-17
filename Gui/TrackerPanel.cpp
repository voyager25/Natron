//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "TrackerPanel.h"

#include <QHBoxLayout>

#include "Engine/TrackerContext.h"

#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/Label.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/TableModelView.h"


using namespace Natron;

struct TrackerPanelPrivate
{
    TrackerPanel* _publicInterface;
    boost::weak_ptr<NodeGui> node;
    boost::weak_ptr<TrackerContext> context;
 
    TableView* view;
    TableModel* model;
    
    Natron::Label* exportLabel;
    QWidget* exportContainer;
    QHBoxLayout* exportLayout;
    ComboBox* exportChoice;
    Button* exportButton;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* addButton;
    Button* removeButton;
    Button* selectAllButton;
    Button* resetTracksButton;
    Button* averageTracksButton;

    
    TrackerPanelPrivate(TrackerPanel* publicI, const boost::shared_ptr<NodeGui>& n)
    : _publicInterface(publicI)
    , node(n)
    , context()
    , view(0)
    , model(0)
    , exportLabel(0)
    , exportContainer(0)
    , exportLayout(0)
    , exportChoice(0)
    , exportButton(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , removeButton(0)
    , selectAllButton(0)
    , resetTracksButton(0)
    , averageTracksButton(0)
    {
        
    }
};

TrackerPanel::TrackerPanel(const boost::shared_ptr<NodeGui>& n,
                           QWidget* parent)
: QWidget(parent)
, _imp(new TrackerPanelPrivate(this, n))
{
}
