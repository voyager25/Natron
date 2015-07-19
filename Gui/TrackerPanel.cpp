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

#include <QApplication>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelection>
#include <QDebug>


#include "Engine/Settings.h"
#include "Engine/TrackerContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"

#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/TableModelView.h"
#include "Gui/Utils.h"
#include "Gui/NodeGui.h"
#include "Gui/TrackerUndoCommand.h"
#include "Gui/NodeGraph.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"

#define NUM_COLS 10

#define COL_ENABLED 0
#define COL_LABEL 1
#define COL_SCRIPT_NAME 2
#define COL_MOTION_MODEL 3
#define COL_CENTER_X 4
#define COL_CENTER_Y 5
#define COL_OFFSET_X 6
#define COL_OFFSET_Y 7
#define COL_CORRELATION 8
#define COL_WEIGHT 9

#define kCornerPinInvertParamName "invert"


using namespace Natron;


class TrackerTableItemDelegate
: public QStyledItemDelegate
{
    TableView* _view;
    TrackerPanel* _panel;
    
public:
    
    explicit TrackerTableItemDelegate(TableView* view,
                               TrackerPanel* panel);
    
private:
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

TrackerTableItemDelegate::TrackerTableItemDelegate(TableView* view,
                                     TrackerPanel* panel)
: QStyledItemDelegate(view)
, _view(view)
, _panel(panel)
{
}

void
TrackerTableItemDelegate::paint(QPainter * painter,
                         const QStyleOptionViewItem & option,
                         const QModelIndex & index) const
{
    assert(index.isValid());
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    TableItem* item = model->item(index);
    assert(item);
    if (!item) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    int col = index.column();
    if (col != COL_CENTER_X &&
        col != COL_CENTER_Y &&
        col != COL_OFFSET_X &&
        col != COL_OFFSET_Y &&
        col != COL_WEIGHT &&
        col != COL_CORRELATION) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    
    int dim;
    Natron::AnimationLevelEnum level = eAnimationLevelNone;
    boost::shared_ptr<KnobI> knob = _panel->getKnobAt(index.row(), index.column(), &dim);
    assert(knob);
    if (knob) {
        level = knob->getAnimationLevel(dim);
    }
    
    bool fillRect = true;
    QBrush brush;
    if (option.state & QStyle::State_Selected) {
        brush = option.palette.highlight();
    } else if (level == eAnimationLevelInterpolatedValue) {
        double r,g,b;
        appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else if (level == eAnimationLevelOnKeyframe) {
        double r,g,b;
        appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else {
        fillRect = false;
    }
    if (fillRect) {
        painter->fillRect( geom, brush);
    }
    
    QPen pen = painter->pen();
    if (!item->flags().testFlag(Qt::ItemIsEditable)) {
        pen.setColor(Qt::black);
    } else {
        double r,g,b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        pen.setColor(col);
    }
    painter->setPen(pen);
    
    QRect textRect( geom.x() + 5,geom.y(),geom.width() - 5,geom.height() );
    QRect r;
    QString data;
    QVariant var = item->data(Qt::DisplayRole);
    if (var.canConvert(QVariant::String)) {
        data = var.toString();
    } else if (var.canConvert(QVariant::Double)) {
        double d = var.toDouble();
        data = QString::number(d);
    } else if (var.canConvert(QVariant::Int)) {
        int i = var.toInt();
        data = QString::number(i);
    }
    
    painter->drawText(textRect,Qt::TextSingleLine,data,&r);
    
}

struct ItemData
{
    TableItem* item;
    boost::weak_ptr<KnobI> knob;
    int dimension;
};

struct TrackDatas
{
    std::vector<ItemData> items;
    boost::weak_ptr<TrackMarker> marker;
};

typedef std::vector< TrackDatas> TrackItems;

struct TrackKeys
{
    std::set<int> userKeys;
    std::set<int> centerKeys;
    bool visible;
};

typedef std::map<boost::weak_ptr<TrackMarker>,TrackKeys  > TrackKeysMap;

struct TrackerPanelPrivate
{
    TrackerPanel* _publicInterface;
    boost::weak_ptr<NodeGui> node;
    boost::weak_ptr<TrackerContext> context;
  
    TrackItems items;
    
    QVBoxLayout* mainLayout;
    
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

    int selectionBlocked;
    
    TrackKeysMap keys;
    
    TrackerPanelPrivate(TrackerPanel* publicI, const boost::shared_ptr<NodeGui>& n)
    : _publicInterface(publicI)
    , node(n)
    , context()
    , items()
    , mainLayout(0)
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
    , selectionBlocked(0)
    , keys()
    {
        context = n->getNode()->getTrackerContext();
        assert(context.lock());
    }
    
    void makeTrackRowItems(const TrackMarker& marker, int row, TrackDatas& data);
    
    void markersToSelection(const std::list<boost::shared_ptr<TrackMarker> >& markers, QItemSelection* selection);
    
    void selectionToMarkers(const QItemSelection& selection, std::list<boost::shared_ptr<TrackMarker> >* markers);
    
    void createCornerPinFromSelection(const std::list<boost::shared_ptr<TrackMarker> > & selection,bool linked,bool useTransformRefFrame,bool invert);
    
    void setVisibleItemKeyframes(const std::set<int>& keys,bool visible, bool emitSignal);
};

TrackerPanel::TrackerPanel(const boost::shared_ptr<NodeGui>& n,
                           QWidget* parent)
: QWidget(parent)
, _imp(new TrackerPanelPrivate(this, n))
{
    boost::shared_ptr<TrackerContext> context = getContext();
    QObject::connect(context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onContextSelectionChanged(int)));
    
    QObject::connect(context.get(), SIGNAL(keyframeSetOnTrack(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onTrackKeyframeSet(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(keyframeRemovedOnTrack(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onTrackKeyframeRemoved(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(allKeyframesRemovedOnTrack(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onTrackAllKeyframesRemoved(boost::shared_ptr<TrackMarker>)));
    
    QObject::connect(context.get(), SIGNAL(keyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker>,int)),
                    this, SLOT(onKeyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(keyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onKeyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(allKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onAllKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>)));
    QObject::connect(context.get(), SIGNAL(multipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker>,std::list<int>)),
                     this, SLOT(onMultipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker>,std::list<int>)));
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    _imp->view = new TableView(this);
    QObject::connect( _imp->view,SIGNAL( deleteKeyPressed() ),this,SLOT( onRemoveButtonClicked() ) );
    QObject::connect( _imp->view,SIGNAL( itemRightClicked(TableItem*) ),this,SLOT( onItemRightClicked(TableItem*) ) );
    TrackerTableItemDelegate* delegate = new TrackerTableItemDelegate(_imp->view,this);
    _imp->view->setItemDelegate(delegate);
    
    _imp->model = new TableModel(0,0,_imp->view);
    QObject::connect( _imp->model,SIGNAL( s_itemChanged(TableItem*) ),this,SLOT( onItemDataChanged(TableItem*) ) );
    _imp->view->setTableModel(_imp->model);
    
    QItemSelectionModel *selectionModel = _imp->view->selectionModel();
    QObject::connect( selectionModel, SIGNAL( selectionChanged(QItemSelection,QItemSelection) ),this,
                     SLOT( onModelSelectionChanged(QItemSelection,QItemSelection) ) );
    
    

    QStringList dimensionNames;
    dimensionNames << tr("Enabled") << tr("Label") << tr("Script-name") << tr("Motion-Model") <<
    tr("Center X") << tr("Center Y") << tr("Offset X") << tr("Offset Y") <<
    tr("Correlation") << tr("Weight");
    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    
    _imp->view->setAttribute(Qt::WA_MacShowFocusRect,0);
    
#if QT_VERSION < 0x050000
    _imp->view->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->view->header()->setStretchLastSection(true);
    
    _imp->mainLayout->addWidget(_imp->view);

    
    _imp->exportLabel = new Natron::Label( tr("Export data"),this);
    _imp->mainLayout->addWidget(_imp->exportLabel);
    _imp->mainLayout->addSpacing(10);
    _imp->exportContainer = new QWidget(this);
    _imp->exportLayout = new QHBoxLayout(_imp->exportContainer);
    _imp->exportLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->exportChoice = new ComboBox(_imp->exportContainer);
    _imp->exportChoice->setToolTip( "<p><b>" + tr("CornerPin (Use current frame):") + "</p></b>"
                                   "<p>" + tr("Warp the image according to the relative transform using the current frame as reference.") + "</p>"
                                   "<p><b>" + tr("CornerPin (Use transform ref frame):") + "</p></b>"
                                   "<p>" + tr("Warp the image according to the relative transform using the "
                                              "reference frame specified in the transform tab.") + "</p>"
                                   "<p><b>" + tr("CornerPin (Stabilize):") + "</p></b>"
                                   "<p>" + tr("Transform the image so that the tracked points do not move.") + "</p>"
                                   //                                      "<p><b>" + tr("Transform (Stabilize):</p></b>"
                                   //                                      "<p>" + tr("Transform the image so that the tracked points do not move.") + "</p>"
                                   //                                      "<p><b>" + tr("Transform (Match-move):</p></b>"
                                   //                                      "<p>" + tr("Transform another image so that it moves to match the tracked points.") + "</p>"
                                   //                                      "<p>" + tr("The linked versions keep a link between the new node and the track, the others just copy"
                                   //                                      " the values.") + "</p>"
                                   );
    std::vector<std::string> choices;
    std::vector<std::string> helps;
    
    choices.push_back(tr("CornerPin (Use current frame. Linked)").toStdString());
    helps.push_back(tr("Warp the image according to the relative transform using the current frame as reference.").toStdString());
    //
    //    choices.push_back(tr("CornerPinOFX (Use transform ref frame. Linked)").toStdString());
    //    helps.push_back(tr("Warp the image according to the relative transform using the "
    //                       "reference frame specified in the transform tab.").toStdString());
    
    choices.push_back(tr("CornerPin (Stabilize. Linked)").toStdString());
    helps.push_back(tr("Transform the image so that the tracked points do not move.").toStdString());
    
    choices.push_back( tr("CornerPin (Use current frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );
    
    choices.push_back(tr("CornerPin (Stabilize. Copy)").toStdString());
    helps.push_back(tr("Same as the linked version except that it copies values instead of "
                       "referencing them via a link to the track").toStdString());
    
    choices.push_back( tr("CornerPin (Use transform ref frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );
    
    
    //    choices.push_back(tr("Transform (Stabilize. Linked)").toStdString());
    //    helps.push_back(tr("Transform the image so that the tracked points do not move.").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Linked)").toStdString());
    //    helps.push_back(tr("Transform another image so that it moves to match the tracked points.").toStdString());
    //
    //    choices.push_back(tr("Transform (Stabilize. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    for (U32 i = 0; i < choices.size(); ++i) {
        _imp->exportChoice->addItem( choices[i].c_str(),QIcon(),QKeySequence(),helps[i].c_str() );
    }
    _imp->exportLayout->addWidget(_imp->exportChoice);
    
    _imp->exportButton = new Button(tr("Export"),_imp->exportContainer);
    QObject::connect( _imp->exportButton,SIGNAL( clicked(bool) ),this,SLOT( onExportButtonClicked() ) );
    _imp->exportLayout->addWidget(_imp->exportButton);
    _imp->exportLayout->addStretch();
    _imp->mainLayout->addWidget(_imp->exportContainer);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->addButton = new Button(QIcon(),"+",_imp->buttonsContainer);
    _imp->addButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->addButton->setToolTip(Natron::convertFromPlainText(tr("Add new."), Qt::WhiteSpaceNormal));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    QObject::connect( _imp->addButton, SIGNAL( clicked(bool) ), this, SLOT( onAddButtonClicked() ) );
    
    _imp->removeButton = new Button(QIcon(),"-",_imp->buttonsContainer);
    _imp->removeButton->setToolTip(Natron::convertFromPlainText(tr("Remove selection."), Qt::WhiteSpaceNormal));
    _imp->removeButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    QObject::connect( _imp->removeButton, SIGNAL( clicked(bool) ), this, SLOT( onRemoveButtonClicked() ) );
    QPixmap selectAll;
    appPTR->getIcon(NATRON_PIXMAP_SELECT_ALL, &selectAll);
    
    _imp->selectAllButton = new Button(QIcon(selectAll),"",_imp->buttonsContainer);
    _imp->selectAllButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->selectAllButton->setToolTip(Natron::convertFromPlainText(tr("Select all."), Qt::WhiteSpaceNormal));
    _imp->buttonsLayout->addWidget(_imp->selectAllButton);
    QObject::connect( _imp->selectAllButton, SIGNAL( clicked(bool) ), this, SLOT( onSelectAllButtonClicked() ) );
    
    _imp->resetTracksButton = new Button("Reset",_imp->buttonsContainer);
    QObject::connect( _imp->resetTracksButton, SIGNAL( clicked(bool) ), this, SLOT( onResetButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->resetTracksButton);
    _imp->resetTracksButton->setToolTip(Natron::convertFromPlainText(tr("Reset selected items."), Qt::WhiteSpaceNormal));
    
    _imp->buttonsLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    
}

void
TrackerPanelPrivate::makeTrackRowItems(const TrackMarker& marker, int row, TrackDatas& data)
{
    ///Enabled
    {
        ItemData d;
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked(marker.isEnabled());
        QObject::connect( checkbox,SIGNAL( toggled(bool) ),_publicInterface,SLOT( onItemEnabledCheckBoxChecked(bool) ) );
        view->setCellWidget(row, COL_ENABLED, checkbox);
        TableItem* newItem = new TableItem;
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        newItem->setToolTip(QObject::tr("When checked, this track will no longer be tracked even if selected"));
        view->setItem(row, COL_ENABLED, newItem);
        view->resizeColumnToContents(COL_ENABLED);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Label
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_LABEL, newItem);
        newItem->setToolTip(QObject::tr("The label of the item as seen in the viewer"));
        newItem->setText(marker.getLabel().c_str());
        view->resizeColumnToContents(COL_LABEL);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Script name
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_SCRIPT_NAME, newItem);
        newItem->setToolTip(QObject::tr("The script-name of the item as exposed to Python scripts"));
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_SCRIPT_NAME);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Motion-model
    {
        ItemData d;
        boost::shared_ptr<Choice_Knob> motionModel = marker.getMotionModelKnob();
        ComboBox* cb = new ComboBox;
        std::vector<std::string> choices,helps;
        TrackerContext::getMotionModelsAndHelps(&choices,&helps);
        cb->setCurrentIndex(motionModel->getValue());
        QObject::connect( cb,SIGNAL( currentIndexChanged(int) ),_publicInterface,SLOT( onItemMotionModelChanged(int) ) );
        assert(choices.size() == helps.size());
        for (std::size_t i = 0; i < choices.size(); ++i) {
            cb->addItem(choices[i].c_str(), QIcon(),QKeySequence(), helps[i].c_str());
        }
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_MOTION_MODEL, newItem);
        newItem->setToolTip(QObject::tr("The motion model to use for tracking"));
        newItem->setText(marker.getScriptName().c_str());
        view->setCellWidget(row, COL_MOTION_MODEL, cb);
        view->resizeColumnToContents(COL_MOTION_MODEL);
        d.item = newItem;
        d.dimension = 0;
        d.knob = motionModel;
        data.items.push_back(d);
    }
    
    //Center X
    boost::shared_ptr<Double_Knob> center = marker.getCenterKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CENTER_X, newItem);
        newItem->setToolTip(QObject::tr("The x coordinate of the center of the track"));
        newItem->setData(Qt::DisplayRole, center->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_X);
        d.item = newItem;
        d.knob = center;
        d.dimension = 0;
        data.items.push_back(d);
    }
    //Center Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CENTER_Y, newItem);
        newItem->setToolTip(QObject::tr("The y coordinate of the center of the track"));
        newItem->setData(Qt::DisplayRole, center->getValue(1));
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_Y);
        d.item = newItem;
        d.knob = center;
        d.dimension = 1;
        data.items.push_back(d);
    }
    ///Offset X
    boost::shared_ptr<Double_Knob> offset = marker.getOffsetKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_OFFSET_X, newItem);
        newItem->setToolTip(QObject::tr("The x offset applied to the search window for the track"));
        newItem->setData(Qt::DisplayRole, offset->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_X);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 0;
        data.items.push_back(d);
    }
    ///Offset Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_OFFSET_Y, newItem);
        newItem->setToolTip(QObject::tr("The y offset applied to the search window for the track"));
        newItem->setData(Qt::DisplayRole, offset->getValue(1));
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_Y);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 1;
        data.items.push_back(d);
    }
    
    ///Correlation
    boost::shared_ptr<Double_Knob> correlation = marker.getCorrelationKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CORRELATION, newItem);
        newItem->setToolTip(QObject::tr(correlation->getHintToolTip().c_str()));
        newItem->setData(Qt::DisplayRole, correlation->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CORRELATION);
        d.item = newItem;
        d.knob = correlation;
        d.dimension = 0;
        data.items.push_back(d);
    }

    
    ///Weight
    boost::shared_ptr<Double_Knob> weight = marker.getWeightKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_WEIGHT, newItem);
        newItem->setToolTip(QObject::tr("The weight determines the amount this marker contributes to the final solution"));
        newItem->setData(Qt::DisplayRole, offset->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_WEIGHT);
        d.item = newItem;
        d.knob = weight;
        d.dimension = 0;
        data.items.push_back(d);
    }

}

TrackerPanel::~TrackerPanel()
{
    
}

void
TrackerPanel::addTableRow(const boost::shared_ptr<TrackMarker> & node)
{
    int newRowIndex = _imp->view->rowCount();
    _imp->model->insertRow(newRowIndex);

    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, newRowIndex, data);
    
    assert((int)_imp->items.size() == newRowIndex);
    _imp->items.push_back(data);
    
    if (!_imp->selectionBlocked) {
        ///select the new item
        QModelIndex newIndex = _imp->model->index(newRowIndex, COL_ENABLED);
        assert( newIndex.isValid() );
        _imp->view->selectionModel()->select(newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

void
TrackerPanel::insertTableRow(const boost::shared_ptr<TrackMarker> & node, int index)
{
    assert(index >= 0);
    
    _imp->model->insertRow(index);
    
    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, index, data);
    
    if (index >= (int)_imp->items.size()) {
        _imp->items.push_back(data);
    } else {
        TrackItems::iterator it = _imp->items.begin();
        std::advance(it, index);
        _imp->items.insert(it, data);
    }
    
    if (!_imp->selectionBlocked) {
        ///select the new item
        QModelIndex newIndex = _imp->model->index(index, COL_ENABLED);
        assert( newIndex.isValid() );
        _imp->view->selectionModel()->select(newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

void
TrackerPanel::blockSelection()
{
    ++_imp->selectionBlocked;
}

void
TrackerPanel::unblockSelection()
{
    if (_imp->selectionBlocked > 0) {
        --_imp->selectionBlocked;
    }
}

int
TrackerPanel::getMarkerRow(const boost::shared_ptr<TrackMarker> & marker) const
{
    int i = 0;
    for (TrackItems::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it, ++i) {
        if (it->marker.lock() == marker) {
            return i;
        }
    }
    return -1;
}

boost::shared_ptr<TrackMarker>
TrackerPanel::getRowMarker(int row) const
{
    if (row < 0 || row >= (int)_imp->items.size()) {
        return boost::shared_ptr<TrackMarker>();
    }
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        if (row == (int)i) {
            return _imp->items[i].marker.lock();
        }
    }
    assert(false);
}

void
TrackerPanel::removeRow(int row)
{
    if (row < 0 || row >= (int)_imp->items.size()) {
        return;
    }
    
    _imp->model->removeRows(row);
    TrackItems::iterator it = _imp->items.begin();
    std::advance(it, row);
    _imp->items.erase(it);

}

void
TrackerPanel::removeMarker(const boost::shared_ptr<TrackMarker> & marker)
{
    int row = getMarkerRow(marker);
    removeRow(row);
}

boost::shared_ptr<TrackerContext>
TrackerPanel::getContext() const
{
    return _imp->context.lock();
}

boost::shared_ptr<NodeGui>
TrackerPanel::getNode() const
{
    return _imp->node.lock();
}

TableItem*
TrackerPanel::getItemAt(int row, int column) const
{
    if (row < 0 || row >= (int)_imp->items.size() || column < 0 || column >= NUM_COLS) {
        return 0;
    }
    return _imp->items[row].items[column].item;
}

boost::shared_ptr<KnobI>
TrackerPanel::getKnobAt(int row, int column, int* dimension) const
{
    if (row < 0 || row >= (int)_imp->items.size() || column < 0 || column >= NUM_COLS) {
        return boost::shared_ptr<KnobI>();
    }
    *dimension = _imp->items[row].items[column].dimension;
    return _imp->items[row].items[column].knob.lock();
}

void
TrackerPanel::getSelectedRows(std::set<int>* rows) const
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    std::list<boost::shared_ptr<Node> > instances;
    QModelIndexList indexes = selection.indexes();
    for (int i = 0; i < indexes.size(); ++i) {
        rows->insert(indexes[i].row());
    }
    
}

void
TrackerPanel::onAddButtonClicked()
{
    makeTrackInternal();
}

void
TrackerPanel::pushUndoCommand(QUndoCommand* command)
{
    boost::shared_ptr<NodeGui> node = getNode();
    if (node) {
        NodeSettingsPanel* panel = node->getSettingPanel();
        assert(panel);
        panel->pushUndoCommand(command);
    }
}

void
TrackerPanel::onRemoveButtonClicked()
{
    std::set<int> rows;
    getSelectedRows(&rows);
    std::list<boost::shared_ptr<TrackMarker> > markers;
    for (std::set<int>::iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<TrackMarker> marker = getRowMarker(*it);
        if (marker) {
            markers.push_back(marker);
        }
    }
    if (!markers.empty()) {
        pushUndoCommand(new RemoveTracksCommand(markers, getContext()));
    }
}

void
TrackerPanel::onSelectAllButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    context->selectAll(TrackerContext::eTrackSelectionInternal);
}

void
TrackerPanel::onResetButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    std::list<boost::shared_ptr<TrackMarker> > markers;
    context->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        (*it)->resetTrack();
    }
}

boost::shared_ptr<TrackMarker>
TrackerPanel::makeTrackInternal()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    boost::shared_ptr<TrackMarker> ret = context->createMarker();
    assert(ret);
    pushUndoCommand(new AddTrackCommand(ret,context));
    return ret;
}

void
TrackerPanel::onAverageButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    std::list<boost::shared_ptr<TrackMarker> > markers;
    context->getSelectedMarkers(&markers);
    if ( markers.empty() ) {
        Natron::warningDialog( tr("Average").toStdString(), tr("No tracks selected").toStdString() );
        return;
    }
    
    boost::shared_ptr<TrackMarker> marker = makeTrackInternal();
    
    boost::shared_ptr<Double_Knob> centerKnob = marker->getCenterKnob();
    
#ifdef AVERAGE_ALSO_PATTERN_QUAD
    boost::shared_ptr<Double_Knob> topLeftKnob = marker->getPatternTopLeftKnob();
    boost::shared_ptr<Double_Knob> topRightKnob = marker->getPatternTopRightKnob();
    boost::shared_ptr<Double_Knob> btmRightKnob = marker->getPatternBtmRightKnob();
    boost::shared_ptr<Double_Knob> btmLeftKnob = marker->getPatternBtmLeftKnob();
#endif
    
    RangeD keyframesRange;
    keyframesRange.min = INT_MAX;
    keyframesRange.max = INT_MIN;
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        boost::shared_ptr<Double_Knob> markCenter = (*it)->getCenterKnob();

        double mini,maxi;
        bool hasKey = markCenter->getFirstKeyFrameTime(0, &mini);
        if (!hasKey) {
            continue;
        }
        if (mini < keyframesRange.min) {
            keyframesRange.min = mini;
        }
        hasKey = markCenter->getLastKeyFrameTime(0, &maxi);
        
        ///both dimensions must have keyframes
        assert(hasKey);
        if (maxi > keyframesRange.max) {
            keyframesRange.max = maxi;
        }
    }
    
    bool hasKeyFrame = keyframesRange.min != INT_MIN && keyframesRange.max != INT_MAX;
    for (double t = keyframesRange.min; t <= keyframesRange.max; t += 1) {
        Natron::Point avgCenter;
        avgCenter.x = avgCenter.y = 0.;

#ifdef AVERAGE_ALSO_PATTERN_QUAD
        Natron::Point avgTopLeft, avgTopRight,avgBtmRight,avgBtmLeft;
        avgTopLeft.x = avgTopLeft.y = avgTopRight.x = avgTopRight.y = avgBtmRight.x = avgBtmRight.y = avgBtmLeft.x = avgBtmLeft.y = 0;
#endif
        
        
        for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
            boost::shared_ptr<Double_Knob> markCenter = (*it)->getCenterKnob();
            
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            boost::shared_ptr<Double_Knob> markTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<Double_Knob> markTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<Double_Knob> markBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<Double_Knob> markBtmLeft = (*it)->getPatternBtmLeftKnob();
#endif
            
            avgCenter.x += markCenter->getValueAtTime(t, 0);
            avgCenter.y += markCenter->getValueAtTime(t, 1);
            
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x += markTopLeft->getValueAtTime(t, 0);
            avgTopLeft.y += markTopLeft->getValueAtTime(t, 1);
            
            avgTopRight.x += markTopRight->getValueAtTime(t, 0);
            avgTopRight.y += markTopRight->getValueAtTime(t, 1);
            
            avgBtmRight.x += markBtmRight->getValueAtTime(t, 0);
            avgBtmRight.y += markBtmRight->getValueAtTime(t, 1);
            
            avgBtmLeft.x += markBtmLeft->getValueAtTime(t, 0);
            avgBtmLeft.y += markBtmLeft->getValueAtTime(t, 1);
#endif
        }
        
        avgCenter.x /= markers.size();
        avgCenter.y /= markers.size();
        
#ifdef AVERAGE_ALSO_PATTERN_QUAD
        avgTopLeft.x /= markers.size();
        avgTopLeft.y /= markers.size();
        
        avgTopRight.x /= markers.size();
        avgTopRight.y /= markers.size();
        
        avgBtmRight.x /= markers.size();
        avgBtmRight.y /= markers.size();
        
        avgBtmLeft.x /= markers.size();
        avgBtmLeft.y /= markers.size();
#endif
        
        if (!hasKeyFrame) {
            centerKnob->setValue(avgCenter.x, 0);
            centerKnob->setValue(avgCenter.y, 1);
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValue(avgTopLeft.x, 0);
            topLeftKnob->setValue(avgTopLeft.y, 1);
            topRightKnob->setValue(avgTopRight.x, 0);
            topRightKnob->setValue(avgTopRight.y, 1);
            btmRightKnob->setValue(avgBtmRight.x, 0);
            btmRightKnob->setValue(avgBtmRight.y, 1);
            btmLeftKnob->setValue(avgBtmLeft.x, 0);
            btmLeftKnob->setValue(avgBtmLeft.y, 1);
#endif
            break;
        } else {
            centerKnob->setValueAtTime(t, avgCenter.x, 0);
            centerKnob->setValueAtTime(t, avgCenter.y, 1);
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValueAtTime(t, avgTopLeft.x, 0);
            topLeftKnob->setValueAtTime(t, avgTopLeft.y, 1);
            topRightKnob->setValueAtTime(t, avgTopRight.x, 0);
            topRightKnob->setValueAtTime(t, avgTopRight.y, 1);
            btmRightKnob->setValueAtTime(t, avgBtmRight.x, 0);
            btmRightKnob->setValueAtTime(t, avgBtmRight.y, 1);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.x, 0);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.y, 1);
#endif
        }
    }
}

static
boost::shared_ptr<Double_Knob>
getCornerPinPoint(Natron::Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString("from%1").arg(index + 1) : QString("to%1").arg(index + 1);
    boost::shared_ptr<KnobI> knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    boost::shared_ptr<Double_Knob>  ret = boost::dynamic_pointer_cast<Double_Knob>(knob);
    assert(ret);
    return ret;
}

void
TrackerPanelPrivate::createCornerPinFromSelection(const std::list<boost::shared_ptr<TrackMarker> > & selection,bool linked,bool useTransformRefFrame,bool invert)
{
    if ( (selection.size() > 4) || selection.empty() ) {
        Natron::errorDialog( QObject::tr("Export").toStdString(),
                            QObject::tr("Export to corner pin needs between 1 and 4 selected tracks.").toStdString() );
        
        return;
    }
    
    boost::shared_ptr<TrackerContext> ctx = context.lock();
    
    boost::shared_ptr<Double_Knob> centers[4];
    int i = 0;
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = selection.begin(); it != selection.end(); ++it, ++i) {
        centers[i] = (*it)->getCenterKnob();
        assert(centers[i]);
    }
    
    boost::shared_ptr<NodeGui> node = _publicInterface->getNode();
    
    GuiAppInstance* app = node->getDagGui()->getGui()->getApp();
    boost::shared_ptr<Natron::Node> cornerPin = app->createNode(CreateNodeArgs(PLUGINID_OFX_CORNERPIN,
                                                                               "",
                                                                               -1, -1,
                                                                               false, //< don't autoconnect
                                                                               INT_MIN,
                                                                               INT_MIN,
                                                                               true,
                                                                               true,
                                                                               true,
                                                                               QString(),
                                                                               CreateNodeArgs::DefaultValuesList(),
                                                                                node->getNode()->getGroup()));
    if (!cornerPin) {
        return;
    }
    
    ///Move the node on the right of the tracker node
    boost::shared_ptr<NodeGuiI> cornerPinGui_i = cornerPin->getNodeGui();
    NodeGui* cornerPinGui = dynamic_cast<NodeGui*>(cornerPinGui_i.get());
    assert(cornerPinGui);

    QPointF mainInstancePos = node->scenePos();
    mainInstancePos = cornerPinGui->mapToParent( cornerPinGui->mapFromScene(mainInstancePos) );
    cornerPinGui->refreshPosition( mainInstancePos.x() + node->getSize().width() * 2, mainInstancePos.y() );
    
    boost::shared_ptr<Double_Knob> toPoints[4];
    boost::shared_ptr<Double_Knob> fromPoints[4];
    
    int timeForFromPoints = useTransformRefFrame ? ctx->getTransformReferenceFrame() : app->getTimeLine()->currentFrame();
    
    for (unsigned int i = 0; i < selection.size(); ++i) {
        fromPoints[i] = getCornerPinPoint(cornerPin.get(), true, i);
        assert(fromPoints[i] && centers[i]);
        for (int j = 0; j < fromPoints[i]->getDimension(); ++j) {
            fromPoints[i]->setValue(centers[i]->getValueAtTime(timeForFromPoints,j), j);
        }
        
        toPoints[i] = getCornerPinPoint(cornerPin.get(), false, i);
        assert(toPoints[i]);
        if (!linked) {
            toPoints[i]->cloneAndUpdateGui(centers[i].get());
        } else {
            Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(centers[i]->getHolder());
            assert(effect);
            
            std::stringstream ss;
            ss << "thisGroup." << effect->getNode()->getFullyQualifiedName() << "." << centers[i]->getName() << ".get()[dimension]";
            std::string expr = ss.str();
            dynamic_cast<KnobI*>(toPoints[i].get())->setExpression(0, expr, false);
            dynamic_cast<KnobI*>(toPoints[i].get())->setExpression(1, expr, false);
        }
    }
    
    ///Disable all non used points
    for (unsigned int i = selection.size(); i < 4; ++i) {
        QString enableName = QString("enable%1").arg(i + 1);
        boost::shared_ptr<KnobI> knob = cornerPin->getKnobByName( enableName.toStdString() );
        assert(knob);
        Bool_Knob* enableKnob = dynamic_cast<Bool_Knob*>( knob.get() );
        assert(enableKnob);
        enableKnob->setValue(false, 0);
    }
    
    if (invert) {
        boost::shared_ptr<KnobI> invertKnob = cornerPin->getKnobByName(kCornerPinInvertParamName);
        assert(invertKnob);
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(invertKnob.get());
        assert(isBool);
        isBool->setValue(true, 0);
    }

}

void
TrackerPanel::onExportButtonClicked()
{
    int index = _imp->exportChoice->activeIndex();
    std::list<boost::shared_ptr<TrackMarker> > selection;
    getContext()->getSelectedMarkers(&selection);
    ///This is the full list, decomment when everything will be possible to do
    //    switch (index) {
    //        case 0:
    //            _imp->createCornerPinFromSelection(selection, true, false);
    //            break;
    //        case 1:
    //            _imp->createCornerPinFromSelection(selection, true, true);
    //            break;
    //        case 2:
    //            _imp->createCornerPinFromSelection(selection, false, false);
    //            break;
    //        case 3:
    //            _imp->createCornerPinFromSelection(selection, false, true);
    //            break;
    //        case 4:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeStabilize);
    //            break;
    //        case 5:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeMatchMove);
    //            break;
    //        case 6:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeStabilize);
    //            break;
    //        case 7:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeMatchMove);
    //            break;
    //        default:
    //            break;
    //    }
    switch (index) {
        case 0:
            _imp->createCornerPinFromSelection(selection, true, false, false);
            break;
        case 1:
            _imp->createCornerPinFromSelection(selection, true, false, true);
            break;
        case 2:
            _imp->createCornerPinFromSelection(selection, false, false, false);
            break;
        case 3:
            _imp->createCornerPinFromSelection(selection, false, false, true);
            break;
        case 4:
            _imp->createCornerPinFromSelection(selection, false, true, false);
            break;
        default:
            break;
    }
}

void
TrackerPanelPrivate::markersToSelection(const std::list<boost::shared_ptr<TrackMarker> >& markers, QItemSelection* selection)
{
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin(); it!=markers.end(); ++it) {
        int row = _publicInterface->getMarkerRow(*it);
        if (row == -1) {
            qDebug() << "Attempt to select invalid marker" << (*it)->getScriptName().c_str();
            continue;
        }
        QModelIndex left = model->index(row, 0);
        QModelIndex right = model->index(row, NUM_COLS - 1);
        assert(left.isValid() && right.isValid());
        QItemSelectionRange range(left,right);
        selection->append(range);
    }

}

void
TrackerPanelPrivate::selectionToMarkers(const QItemSelection& selection, std::list<boost::shared_ptr<TrackMarker> >* markers)
{
    QModelIndexList indexes = selection.indexes();
    for (int i = 0; i < indexes.size(); ++i) {
        
        //Check that the item is valid
        assert(indexes[i].isValid() && indexes[i].row() >= 0 && indexes[i].row() < (int)items.size() && indexes[i].column() >= 0 && indexes[i].column() < NUM_COLS);
        
        //Check that the items vector is in sync with the model
        assert(items[indexes[i].row()].items[indexes[i].column()].item == model->item(indexes[i]));
        
        boost::shared_ptr<TrackMarker> marker = items[indexes[i].row()].marker.lock();
        if (marker) {
            markers->push_back(marker);
        }
    }
}

void
TrackerPanel::onContextSelectionChanged(int reason)
{
    TrackerContext::TrackSelectionReason selectionReason = (TrackerContext::TrackSelectionReason)reason;
    if (selectionReason == TrackerContext::eTrackSelectionSettingsPanel) {
        //avoid recursions
        return;
    }
    std::list<boost::shared_ptr<TrackMarker> > selection;
    getContext()->getSelectedMarkers(&selection);
    selectInternal(selection, reason);
}

void
TrackerPanel::onModelSelectionChanged(const QItemSelection & /*oldSelection*/,const QItemSelection & newSelection)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->selectionToMarkers(newSelection, &markers);
    clearAndSelectTracks(markers, TrackerContext::eTrackSelectionSettingsPanel);
}

void
TrackerPanel::clearAndSelectTracks(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason)
{
    selectInternal(markers, reason);
}

void
TrackerPanel::selectInternal(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason)
{
    
    
    if (!_imp->selectionBlocked) {
        ///select the new item
        QItemSelection selection;
        _imp->markersToSelection(markers, &selection);
        _imp->view->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    
    TrackerContext::TrackSelectionReason selectionReason = (TrackerContext::TrackSelectionReason)reason;
    
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    context->beginEditSelection();
    context->clearSelection(selectionReason);
    context->addTracksToSelection(markers, selectionReason);
    context->endEditSelection(selectionReason);
}

void
TrackerPanel::onItemRightClicked(TableItem* /*item*/)
{
    
}

void
TrackerPanel::onItemDataChanged(TableItem* item)
{
    int time = _imp->node.lock()->getDagGui()->getGui()->getApp()->getTimeLine()->currentFrame();
    
    for (std::size_t it = 0; it < _imp->items.size(); ++it) {
        for (std::size_t i = 0; i < _imp->items[it].items.size(); ++i) {
            if (_imp->items[it].items[i].item == item) {
                
                boost::shared_ptr<TrackMarker> marker = _imp->items[it].marker.lock();
                if (!marker) {
                    return;
                }
                switch (i) {
                    case COL_ENABLED:
                    case COL_MOTION_MODEL:
                        //Columns with a custom cell widget are handled in their respective slots
                       break;
                    case COL_LABEL: {
                        std::string label = item->data(Qt::DisplayRole).toString().toStdString();
                        marker->setLabel(label);
                    }   break;
                    case COL_CENTER_X:
                    case COL_CENTER_Y:
                    case COL_OFFSET_X:
                    case COL_OFFSET_Y:
                    case COL_WEIGHT:
                    case COL_CORRELATION: {
                        boost::shared_ptr<Double_Knob> knob = boost::dynamic_pointer_cast<Double_Knob>(_imp->items[it].items[i].knob.lock());
                        assert(knob);
                        int dim = _imp->items[it].items[i].dimension;
                        double value = item->data(Qt::DisplayRole).toDouble();
                        if (knob->isAnimationEnabled() && knob->isAnimated(dim)) {
                            knob->setValueAtTime(time, value, dim);
                        } else {
                            knob->setValue(value, dim);
                        }
                        
                    }   break;
                    case COL_SCRIPT_NAME:
                        //This is not editable
                        break;
                }
            }
        }
    }
}

void
TrackerPanel::onItemEnabledCheckBoxChecked(bool checked)
{
    QCheckBox* widget = qobject_cast<QCheckBox*>(sender());
    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_ENABLED);
        if (widget == cellW) {
            boost::shared_ptr<TrackMarker> marker = _imp->items[i].marker.lock();
            marker->setEnabled(checked);
            break;
        }
    }
}

void
TrackerPanel::onItemMotionModelChanged(int index)
{
    ComboBox* widget = qobject_cast<ComboBox*>(sender());
    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_ENABLED);
        if (widget == cellW) {
            boost::shared_ptr<TrackMarker> marker = _imp->items[i].marker.lock();
            marker->getMotionModelKnob()->setValue(index, 0);
            break;
        }
    }
}

void
TrackerPanel::onTrackKeyframeSet(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onTrackKeyframeRemoved(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onTrackAllKeyframesRemoved(const boost::shared_ptr<TrackMarker>& marker)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onKeyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onKeyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onAllKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanel::onMultipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, const std::list<int>& keys)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
}

void
TrackerPanelPrivate::setVisibleItemKeyframes(const std::set<int>& keyframes,bool visible, bool emitSignal)
{
    std::list<SequenceTime> keys;
    for (std::set<int>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
        keys.push_back(*it2);
    }
    if (!visible) {
        node.lock()->getNode()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keys, emitSignal);
    } else {
        node.lock()->getNode()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(keys, emitSignal);
    }

}
