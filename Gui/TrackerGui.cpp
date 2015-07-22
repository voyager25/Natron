//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "TrackerGui.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QTextEdit>
#include <QKeyEvent>
#include <QPixmap>
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/TrackerContext.h"
#include "Engine/TimeLine.h"

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/FromQtEnums.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/NodeGui.h"
#include "Gui/Utils.h"
#include "Gui/TrackerPanel.h"
#include "Gui/NodeGraph.h"

#define POINT_SIZE 5
#define CROSS_SIZE 6
#define POINT_TOLERANCE 6
#define ADDTRACK_SIZE 5
#define HANDLE_SIZE 6


using namespace Natron;


enum TrackerMouseStateEnum
{
    eMouseStateIdle = 0,
    eMouseStateDraggingCenter,
    eMouseStateDraggingOffset,
    
    eMouseStateDraggingInnerTopLeft,
    eMouseStateDraggingInnerTopRight,
    eMouseStateDraggingInnerBtmLeft,
    eMouseStateDraggingInnerBtmRight,
    eMouseStateDraggingInnerTopMid,
    eMouseStateDraggingInnerMidRight,
    eMouseStateDraggingInnerBtmMid,
    eMouseStateDraggingInnerMidLeft,
    
    eMouseStateDraggingOuterTopLeft,
    eMouseStateDraggingOuterTopRight,
    eMouseStateDraggingOuterBtmLeft,
    eMouseStateDraggingOuterBtmRight,
    eMouseStateDraggingOuterTopMid,
    eMouseStateDraggingOuterMidRight,
    eMouseStateDraggingOuterBtmMid,
    eMouseStateDraggingOuterMidLeft
};

enum TrackerDrawStateEnum
{
    eDrawStateInactive = 0,
    eDrawStateHoveringCenter,
    
    eDrawStateHoveringInnerTopLeft,
    eDrawStateHoveringInnerTopRight,
    eDrawStateHoveringInnerBtmLeft,
    eDrawStateHoveringInnerBtmRight,
    eDrawStateHoveringInnerTopMid,
    eDrawStateHoveringInnerMidRight,
    eDrawStateHoveringInnerBtmMid,
    eDrawStateHoveringInnerMidLeft,
    
    eDrawStateHoveringOuterTopLeft,
    eDrawStateHoveringOuterTopRight,
    eDrawStateHoveringOuterBtmLeft,
    eDrawStateHoveringOuterBtmRight,
    eDrawStateHoveringOuterTopMid,
    eDrawStateHoveringOuterMidRight,
    eDrawStateHoveringOuterBtmMid,
    eDrawStateHoveringOuterMidLeft
};


struct TrackerGuiPrivate
{
    boost::shared_ptr<TrackerPanelV1> panelv1;
    TrackerPanel* panel;
    ViewerTab* viewer;
    QWidget* buttonsBar;
    QHBoxLayout* buttonsLayout;
    Button* addTrackButton;
    Button* trackBwButton;
    Button* trackPrevButton;
    Button* stopTrackingButton;
    Button* trackNextButton;
    Button* trackFwButton;
    Button* clearAllAnimationButton;
    Button* clearBwAnimationButton;
    Button* clearFwAnimationButton;
    Button* updateViewerButton;
    Button* centerViewerButton;
    
    Button* createKeyOnMoveButton;
    Button* setKeyFrameButton;
    Button* removeKeyFrameButton;
    Button* removeAllKeyFramesButton;
    Button* resetOffsetButton;
    Button* resetTrackButton;
    Button* showCorrelationButton;

    
    bool clickToAddTrackEnabled;
    QPointF lastMousePos;
    QRectF selectionRectangle;
    int controlDown;
    
    TrackerMouseStateEnum eventState;
    TrackerDrawStateEnum hoverState;
    
    boost::shared_ptr<TrackMarker> interactMarker,hoverMarker;

    TrackerGuiPrivate(const boost::shared_ptr<TrackerPanelV1> & panelv1,
                      TrackerPanel* panel,
                      ViewerTab* parent)
    : panelv1(panelv1)
    , panel(panel)
    , viewer(parent)
    , buttonsBar(NULL)
    , buttonsLayout(NULL)
    , addTrackButton(NULL)
    , trackBwButton(NULL)
    , trackPrevButton(NULL)
    , stopTrackingButton(NULL)
    , trackNextButton(NULL)
    , trackFwButton(NULL)
    , clearAllAnimationButton(NULL)
    , clearBwAnimationButton(NULL)
    , clearFwAnimationButton(NULL)
    , updateViewerButton(NULL)
    , centerViewerButton(NULL)
    , createKeyOnMoveButton(0)
    , setKeyFrameButton(0)
    , removeKeyFrameButton(0)
    , removeAllKeyFramesButton(0)
    , resetOffsetButton(0)
    , resetTrackButton(0)
    , showCorrelationButton(0)
    , clickToAddTrackEnabled(false)
    , lastMousePos()
    , selectionRectangle()
    , controlDown(0)
    , eventState(eMouseStateIdle)
    , hoverState(eDrawStateInactive)
    , interactMarker()
    {
    }
};

TrackerGui::TrackerGui(TrackerPanel* panel,
           ViewerTab* parent)
: QObject()
, _imp(new TrackerGuiPrivate(boost::shared_ptr<TrackerPanelV1>(), panel, parent))

{
    createGui();
}

TrackerGui::TrackerGui(const boost::shared_ptr<TrackerPanelV1> & panel,
                       ViewerTab* parent)
: QObject()
, _imp(new TrackerGuiPrivate(panel, 0, parent))
{
    createGui();
}

void
TrackerGui::createGui()
{
    assert(_imp->viewer);
    
    QObject::connect( _imp->viewer->getViewer(),SIGNAL( selectionRectangleChanged(bool) ),this,SLOT( updateSelectionFromSelectionRectangle(bool) ) );
    QObject::connect( _imp->viewer->getViewer(), SIGNAL( selectionCleared() ), this, SLOT( onSelectionCleared() ) );
    
    if (_imp->panelv1) {
        QObject::connect(_imp->panelv1.get(), SIGNAL(trackingEnded()), this, SLOT(onTrackingEnded()));
    }
    
    _imp->buttonsBar = new QWidget(_imp->viewer);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsBar);
    _imp->buttonsLayout->setContentsMargins(3, 2, 0, 0);
    
    QPixmap pixAdd;
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_TRACK,&pixAdd);
    
    _imp->addTrackButton = new Button(QIcon(pixAdd),"",_imp->buttonsBar);
    _imp->addTrackButton->setCheckable(true);
    _imp->addTrackButton->setChecked(false);
    _imp->addTrackButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->addTrackButton->setToolTip(Natron::convertFromPlainText(tr("When enabled you can add new tracks "
                                                                     "by clicking on the Viewer. "
                                                                     "Holding the Control + Alt keys is the "
                                                                     "same as pressing this button."),
                                                                  Qt::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->addTrackButton);
    QObject::connect( _imp->addTrackButton, SIGNAL( clicked(bool) ), this, SLOT( onAddTrackClicked(bool) ) );
    QPixmap pixPrev,pixNext,pixClearAll,pixClearBw,pixClearFw,pixUpdateViewerEnabled,pixUpdateViewerDisabled,pixStop;
    QPixmap bwEnabled,bwDisabled,fwEnabled,fwDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_REWIND_DISABLED, &bwDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_REWIND_ENABLED, &bwEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS, &pixPrev);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT, &pixNext);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_DISABLED, &fwDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_ENABLED, &fwEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION, &pixClearAll);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION, &pixClearBw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION, &pixClearFw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE, &pixUpdateViewerEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_REFRESH, &pixUpdateViewerDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_STOP, &pixStop);
    
    QIcon bwIcon;
    bwIcon.addPixmap(bwEnabled,QIcon::Normal,QIcon::On);
    bwIcon.addPixmap(bwDisabled,QIcon::Normal,QIcon::Off);
    
    QWidget* trackPlayer = new QWidget(_imp->buttonsBar);
    QHBoxLayout* trackPlayerLayout = new QHBoxLayout(trackPlayer);
    trackPlayerLayout->setContentsMargins(0, 0, 0, 0);
    trackPlayerLayout->setSpacing(0);
    
    _imp->trackBwButton = new Button(bwIcon,"",_imp->buttonsBar);
    _imp->trackBwButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackBwButton->setToolTip("<p>" + tr("Track selected tracks backward until left bound of the timeline.") +
                                    "</p><p><b>" + tr("Keyboard shortcut:") + " Z</b></p>");
    _imp->trackBwButton->setCheckable(true);
    _imp->trackBwButton->setChecked(false);
    QObject::connect( _imp->trackBwButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackBwClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackBwButton);
    
    _imp->trackPrevButton = new Button(QIcon(pixPrev),"",_imp->buttonsBar);
    _imp->trackPrevButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackPrevButton->setToolTip("<p>" + tr("Track selected tracks on the previous frame.") +
                                      "</p><p><b>" + tr("Keyboard shortcut:") + " X</b></p>");
    QObject::connect( _imp->trackPrevButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackPrevClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackPrevButton);
    
    _imp->stopTrackingButton = new Button(QIcon(pixStop),"",_imp->buttonsBar);
    _imp->stopTrackingButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->stopTrackingButton->setToolTip("<p>" + tr("Stop the ongoing tracking if any")  +
                                         "</p><p><b>" + tr("Keyboard shortcut:") + " Escape</b></p>");
    QObject::connect( _imp->stopTrackingButton,SIGNAL( clicked(bool) ),this,SLOT( onStopButtonClicked() ) );
    trackPlayerLayout->addWidget(_imp->stopTrackingButton);
    
    _imp->trackNextButton = new Button(QIcon(pixNext),"",_imp->buttonsBar);
    _imp->trackNextButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackNextButton->setToolTip("<p>" + tr("Track selected tracks on the next frame.") +
                                      "</p><p><b>" + tr("Keyboard shortcut:") + " C</b></p>");
    QObject::connect( _imp->trackNextButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackNextClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackNextButton);
    
    
    QIcon fwIcon;
    fwIcon.addPixmap(fwEnabled,QIcon::Normal,QIcon::On);
    fwIcon.addPixmap(fwDisabled,QIcon::Normal,QIcon::Off);
    _imp->trackFwButton = new Button(fwIcon,"",_imp->buttonsBar);
    _imp->trackFwButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackFwButton->setToolTip("<p>" + tr("Track selected tracks forward until right bound of the timeline.") +
                                    "</p><p><b>" + tr("Keyboard shortcut:") + " V</b></p>");
    _imp->trackFwButton->setCheckable(true);
    _imp->trackFwButton->setChecked(false);
    QObject::connect( _imp->trackFwButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackFwClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackFwButton);
    
    _imp->buttonsLayout->addWidget(trackPlayer);


    QWidget* clearAnimationContainer = new QWidget(_imp->buttonsBar);
    QHBoxLayout* clearAnimationLayout = new QHBoxLayout(clearAnimationContainer);
    clearAnimationLayout->setContentsMargins(0, 0, 0, 0);
    clearAnimationLayout->setSpacing(0);
    
    _imp->clearAllAnimationButton = new Button(QIcon(pixClearAll),"",_imp->buttonsBar);
    _imp->clearAllAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearAllAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear all animation for selected tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearAllAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearAllAnimationClicked() ) );
    clearAnimationLayout->addWidget(_imp->clearAllAnimationButton);
    
    _imp->clearBwAnimationButton = new Button(QIcon(pixClearBw),"",_imp->buttonsBar);
    _imp->clearBwAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearBwAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear animation backward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearBwAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearBwAnimationClicked() ) );
    clearAnimationLayout->addWidget(_imp->clearBwAnimationButton);
    
    _imp->clearFwAnimationButton = new Button(QIcon(pixClearFw),"",_imp->buttonsBar);
    _imp->clearFwAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearFwAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear animation forward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearFwAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearFwAnimationClicked() ) );
    clearAnimationLayout->addWidget(_imp->clearFwAnimationButton);
    
    _imp->buttonsLayout->addWidget(clearAnimationContainer);
    
    QIcon updateViewerIC;
    updateViewerIC.addPixmap(pixUpdateViewerEnabled,QIcon::Normal,QIcon::On);
    updateViewerIC.addPixmap(pixUpdateViewerDisabled,QIcon::Normal,QIcon::Off);
    _imp->updateViewerButton = new Button(updateViewerIC,"",_imp->buttonsBar);
    _imp->updateViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->updateViewerButton->setCheckable(true);
    _imp->updateViewerButton->setChecked(true);
    _imp->updateViewerButton->setDown(true);
    _imp->updateViewerButton->setToolTip(Natron::convertFromPlainText(tr("Update viewer during tracking for each frame instead of just the tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->updateViewerButton,SIGNAL( clicked(bool) ),this,SLOT( onUpdateViewerClicked(bool) ) );
    _imp->buttonsLayout->addWidget(_imp->updateViewerButton);

    QPixmap centerViewerPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK, &centerViewerPix);
    
    _imp->centerViewerButton = new Button(QIcon(centerViewerPix),"",_imp->buttonsBar);
    _imp->centerViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->centerViewerButton->setCheckable(true);
    _imp->centerViewerButton->setChecked(false);
    _imp->centerViewerButton->setDown(false);
    _imp->centerViewerButton->setToolTip(Natron::convertFromPlainText(tr("Center the viewer on selected tracks during tracking."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->centerViewerButton,SIGNAL( clicked(bool) ),this,SLOT( onCenterViewerButtonClicked(bool) ) );
    _imp->buttonsLayout->addWidget(_imp->centerViewerButton);

    
    if (_imp->panel) {
        /// This is for v2 only
        QPixmap createKeyOnMovePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE, &createKeyOnMovePix);
        
        _imp->createKeyOnMoveButton = new Button(QIcon(createKeyOnMovePix), "", _imp->buttonsBar);
        _imp->createKeyOnMoveButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->createKeyOnMoveButton->setToolTip(Natron::convertFromPlainText(tr("When enabled, adjusting a track on the viewer will create a new keyframe"), Qt::WhiteSpaceNormal));
        _imp->createKeyOnMoveButton->setCheckable(true);
        _imp->createKeyOnMoveButton->setChecked(true);
        _imp->createKeyOnMoveButton->setDown(true);
        QObject::connect( _imp->createKeyOnMoveButton,SIGNAL( clicked(bool) ),this,SLOT( onCreateKeyOnMoveButtonClicked(bool) ) );
        _imp->buttonsLayout->addWidget(_imp->createKeyOnMoveButton);
        
        QPixmap showCorrPix,hideCorrPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_SHOW_TRACK_ERROR, &showCorrPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_HIDE_TRACK_ERROR, &hideCorrPix);
        QIcon corrIc;
        corrIc.addPixmap(showCorrPix, QIcon::Normal, QIcon::On);
        corrIc.addPixmap(hideCorrPix, QIcon::Normal, QIcon::Off);
        _imp->showCorrelationButton = new Button(corrIc, "", _imp->buttonsBar);
        _imp->showCorrelationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->showCorrelationButton->setToolTip(Natron::convertFromPlainText(tr("When enabled, the correlation score of each tracked frame will be displayed on "
                                                                                "the viewer, with lower correlations close to green and greater correlations "
                                                                                "close to red."), Qt::WhiteSpaceNormal));
        _imp->showCorrelationButton->setCheckable(true);
        _imp->showCorrelationButton->setChecked(false);
        _imp->showCorrelationButton->setDown(false);
        QObject::connect( _imp->showCorrelationButton,SIGNAL( clicked(bool) ),this,SLOT( onShowCorrelationButtonClicked(bool) ) );
        _imp->buttonsLayout->addWidget(_imp->showCorrelationButton);
        
        QWidget* keyframeContainer = new QWidget(_imp->buttonsBar);
        QHBoxLayout* keyframeLayout = new QHBoxLayout(keyframeContainer);
        keyframeLayout->setContentsMargins(0, 0, 0, 0);
        keyframeLayout->setSpacing(0);
        
        QPixmap addKeyPix,removeKeyPix,resetOffsetPix,removeAllUserKeysPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_USER_KEY, &addKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_USER_KEY, &removeKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESET_TRACK_OFFSET, &resetOffsetPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESET_USER_KEYS, &removeAllUserKeysPix);
        
        _imp->setKeyFrameButton = new Button(QIcon(addKeyPix), "", keyframeContainer);
        _imp->setKeyFrameButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->setKeyFrameButton->setToolTip(Natron::convertFromPlainText(tr("Set a keyframe for the pattern for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->setKeyFrameButton,SIGNAL( clicked(bool) ),this,SLOT( onSetKeyframeButtonClicked() ) );
        keyframeLayout->addWidget(_imp->setKeyFrameButton);
        
        _imp->removeKeyFrameButton = new Button(QIcon(removeKeyPix), "", keyframeContainer);
        _imp->removeKeyFrameButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->removeKeyFrameButton->setToolTip(Natron::convertFromPlainText(tr("Remove a keyframe for the pattern for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->removeKeyFrameButton,SIGNAL( clicked(bool) ),this,SLOT( onRemoveKeyframeButtonClicked() ) );
        keyframeLayout->addWidget(_imp->removeKeyFrameButton);
        
        _imp->removeAllKeyFramesButton = new Button(QIcon(removeAllUserKeysPix), "", keyframeContainer);
        _imp->removeAllKeyFramesButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->removeAllKeyFramesButton->setToolTip(Natron::convertFromPlainText(tr("Remove all keyframes for the pattern for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->removeAllKeyFramesButton,SIGNAL( clicked(bool) ),this,SLOT( onRemoveAnimationButtonClicked() ) );
        keyframeLayout->addWidget(_imp->removeAllKeyFramesButton);
        
        _imp->buttonsLayout->addWidget(keyframeContainer);
        
        _imp->resetOffsetButton = new Button(QIcon(resetOffsetPix), "", _imp->buttonsBar);
        _imp->resetOffsetButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->resetOffsetButton->setToolTip(Natron::convertFromPlainText(tr("Resets the offset for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->resetOffsetButton,SIGNAL( clicked(bool) ),this,SLOT( onResetOffsetButtonClicked() ) );
        _imp->buttonsLayout->addWidget(_imp->resetOffsetButton);
        
        _imp->resetTrackButton = new Button(QIcon(), "Reset track", _imp->buttonsBar);
        _imp->resetTrackButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->resetTrackButton->setToolTip(Natron::convertFromPlainText(tr("Resets animation for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->resetTrackButton,SIGNAL( clicked(bool) ),this,SLOT( onResetTrackButtonClicked() ) );
        _imp->buttonsLayout->addWidget(_imp->resetTrackButton);
        
       

        
    }
    

    
    _imp->buttonsLayout->addStretch();
}

TrackerGui::~TrackerGui()
{
}

QWidget*
TrackerGui::getButtonsBar() const
{
    return _imp->buttonsBar;
}

void
TrackerGui::onAddTrackClicked(bool clicked)
{
    _imp->clickToAddTrackEnabled = !_imp->clickToAddTrackEnabled;
    _imp->addTrackButton->setDown(clicked);
    _imp->addTrackButton->setChecked(clicked);
    _imp->viewer->getViewer()->redraw();
}

static QPointF computeMidPointExtent(const QPointF& prev, const QPointF& next, const QPointF& point, double handleSize)
{
    Natron::Point leftDeriv,rightDeriv;
    leftDeriv.x = prev.x() - point.x();
    leftDeriv.y = prev.y() - point.y();
    
    rightDeriv.x = next.x() - point.x();
    rightDeriv.y = next.y() - point.y();
    double derivNorm = std::sqrt((rightDeriv.x - leftDeriv.x) * (rightDeriv.x - leftDeriv.x) + (rightDeriv.y - leftDeriv.y) * (rightDeriv.y - leftDeriv.y));
    QPointF ret;
    if (derivNorm == 0) {
        double norm = std::sqrt((leftDeriv.x - point.x()) * (leftDeriv.x - point.x()) + (leftDeriv.y - point.y()) * (leftDeriv.y  - point.y()));
        if (norm != 0) {
            ret.rx() = point.x() + ((leftDeriv.y - point.y()) / norm) * handleSize;
            ret.ry() = point.y() - ((leftDeriv.x - point.x()) / norm) * handleSize;
            return ret;
        } else {
            return QPointF(0,0);
        }

    } else {
        ret.rx() = point.x() + ((rightDeriv.y - leftDeriv.y) / derivNorm) * handleSize;
        ret.ry() = point.y() - ((rightDeriv.x - leftDeriv.x) / derivNorm) * handleSize;
    }
    return ret;
}

void
TrackerGui::drawOverlays(double time,
                         double scaleX,
                         double scaleY) const
{
    double pixelScaleX, pixelScaleY;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScaleX, pixelScaleY);
    double viewportSize[2];
    viewer->getViewportSize(viewportSize[0], viewportSize[1]);
    
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
        
        if (_imp->panelv1) {
            ///For each instance: <pointer,selected ? >
            const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
            for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
                
                boost::shared_ptr<Natron::Node> instance = it->first.lock();
                
                if (instance->isNodeDisabled()) {
                    continue;
                }
                if (it->second) {
                    ///The track is selected, use the plug-ins interact
                    Natron::EffectInstance* effect = instance->getLiveInstance();
                    assert(effect);
                    effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                    effect->drawOverlay_public(time, scaleX,scaleY);
                } else {
                    ///Draw a custom interact, indicating the track isn't selected
                    boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
                    assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
                    Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
                    assert(dblKnob);
                    
                    //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        if (l == 0) {
                            glColor4d(0., 0., 0., 1.);
                        } else {
                            glColor4f(1., 1., 1., 1.);
                        }
                        
                        double x = dblKnob->getValue(0);
                        double y = dblKnob->getValue(1);
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x,y);
                        glEnd();
                        
                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);
                        
                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
                }
            }
            
            
        } // if (_imp->panelv1) {
        else {
            assert(_imp->panel);
            double markerColor[3];
            if (!_imp->panel->getNode()->getOverlayColor(&markerColor[0], &markerColor[1], &markerColor[2])) {
                markerColor[0] = markerColor[1] = markerColor[2] = 0.8;
            }
            
            std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
            std::list<boost::shared_ptr<TrackMarker> > selectedMarkers;
            
            boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
            context->getSelectedMarkers(&selectedMarkers);
            context->getAllMarkers(&allMarkers);
            
            for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
                if (!(*it)->isEnabled()) {
                    continue;
                }
                std::list<boost::shared_ptr<TrackMarker> >::iterator foundSelected = std::find(selectedMarkers.begin(),selectedMarkers.end(),*it);
                bool isSelected = foundSelected != selectedMarkers.end();
                
                boost::shared_ptr<Double_Knob> centerKnob = (*it)->getCenterKnob();
                boost::shared_ptr<Double_Knob> offsetKnob = (*it)->getOffsetKnob();
                boost::shared_ptr<Double_Knob> ptnTopLeft = (*it)->getPatternTopLeftKnob();
                boost::shared_ptr<Double_Knob> ptnTopRight = (*it)->getPatternTopRightKnob();
                boost::shared_ptr<Double_Knob> ptnBtmRight = (*it)->getPatternBtmRightKnob();
                boost::shared_ptr<Double_Knob> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
                boost::shared_ptr<Double_Knob> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
                boost::shared_ptr<Double_Knob> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
                
                if (!isSelected) {
                    ///Draw a custom interact, indicating the track isn't selected
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
                    glLineWidth(1.5f);
                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        if (l == 0) {
                            glColor4d(0., 0., 0., 1.);
                        } else {
                            glColor4f(markerColor[0], markerColor[1], markerColor[2], 1.);
                        }
                        
                        
                        double x = centerKnob->getValueAtTime(time,0);
                        double y = centerKnob->getValueAtTime(time,1);
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x,y);
                        glEnd();
                        
                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);
                        
                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
                } else { // if (!isSelected) {
                    
                    GLdouble projection[16];
                    glGetDoublev( GL_PROJECTION_MATRIX, projection);
                    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                    shadow.x = 2./(projection[0] * viewportSize[0]);
                    shadow.y = 2./(projection[5] * viewportSize[1]);
                    
                    QPointF offset,center,topLeft,topRight,btmRight,btmLeft;
                    QPointF searchTopLeft,searchTopRight,searchBtmLeft,searchBtmRight;
                    center.rx() = centerKnob->getValueAtTime(time, 0);
                    center.ry() = centerKnob->getValueAtTime(time, 1);
                    offset.rx() = offsetKnob->getValueAtTime(time, 0);
                    offset.ry() = offsetKnob->getValueAtTime(time, 1);
                    
                    topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchTopLeft.rx() = searchBtmLeft.x();
                    searchTopLeft.ry() = searchTopRight.y();
                    
                    searchBtmRight.rx() = searchTopRight.x();
                    searchBtmRight.ry() = searchBtmLeft.y();
                    
                    QPointF innerMidLeft((btmLeft.x() + topLeft.x()) / 2., (btmLeft.y() + topLeft.y()) / 2.),
                    innerMidTop((topLeft.x() + topRight.x()) / 2., (topLeft.y() + topRight.y()) / 2.),
                    innerMidRight((btmRight.x() + topRight.x()) / 2., (btmRight.y() + topRight.y()) / 2.),
                    innerMidBtm((btmLeft.x() + btmRight.x()) / 2., (btmLeft.y() + btmRight.y()) / 2.),
                    outterMidLeft((searchBtmLeft.x() + searchTopLeft.x()) / 2., (searchBtmLeft.y() + searchTopLeft.y()) / 2.),
                    outterMidTop((searchTopLeft.x() + searchTopRight.x()) / 2., (searchTopLeft.y() + searchTopRight.y()) / 2.),
                    outterMidRight((searchBtmRight.x() + searchTopRight.x()) / 2., (searchBtmRight.y() + searchTopRight.y()) / 2.),
                    outterMidBtm((searchBtmLeft.x() + searchBtmRight.x()) / 2., (searchBtmLeft.y() + searchBtmRight.y()) / 2.);
                    
                    double handleSize = HANDLE_SIZE * pixelScaleX;
                    
                    QPointF innerMidLeftExt = computeMidPointExtent(topLeft, btmLeft, innerMidLeft, handleSize);
                    QPointF innerMidRightExt = computeMidPointExtent(btmRight, topRight, innerMidRight, handleSize);
                    QPointF innerMidTopExt = computeMidPointExtent(topRight, topLeft, innerMidTop, handleSize);
                    QPointF innerMidBtmExt = computeMidPointExtent(btmLeft, btmRight, innerMidBtm, handleSize);
                    
                    QPointF outterMidLeftExt = computeMidPointExtent(searchTopLeft, searchBtmLeft, outterMidLeft, handleSize);
                    QPointF outterMidRightExt = computeMidPointExtent(searchBtmRight, searchTopRight, outterMidRight, handleSize);
                    QPointF outterMidTopExt = computeMidPointExtent(searchTopRight, searchTopLeft, outterMidTop, handleSize);
                    QPointF outterMidBtmExt = computeMidPointExtent(searchBtmLeft,searchBtmRight, outterMidBtm, handleSize);
                    
                    std::string name = (*it)->getLabel();

                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        glBegin(GL_LINE_LOOP);
                        glVertex2d(topLeft.x(), topLeft.y());
                        glVertex2d(topRight.x(), topRight.y());
                        glVertex2d(btmRight.x(), btmRight.y());
                        glVertex2d(btmLeft.x(), btmLeft.y());
                        glEnd();
                        
                        glBegin(GL_LINE_LOOP);
                        glVertex2d(searchTopLeft.x(),searchTopLeft.y());
                        glVertex2d(searchTopRight.x(), searchTopRight.y());
                        glVertex2d(searchBtmRight.x(), searchBtmRight.y());
                        glVertex2d(searchBtmLeft.x(), searchBtmRight.y());
                        glEnd();
                        
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        
                        ///draw center
                        if ( (_imp->hoverState == eDrawStateHoveringCenter) || (_imp->eventState == eMouseStateDraggingCenter)) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(center.x(), center.y());
                        
                        if (offset.x() != 0 || offset.y() != 0) {
                            glVertex2d(center.x() + offset.x(), center.y() + offset.y());
                        }
                        
                        //////DRAWING INNER POINTS
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmLeft) || (_imp->eventState == eMouseStateDraggingInnerBtmLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(btmLeft.x(), btmLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidBtm.x(), innerMidBtm.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmRight) || (_imp->eventState == eMouseStateDraggingInnerBtmRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(btmRight.x(), btmRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidLeft.x(), innerMidLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidRight.x(), innerMidRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopLeft) || (_imp->eventState == eMouseStateDraggingInnerTopLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(topLeft.x(), topLeft.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidTop.x(), innerMidTop.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopRight) || (_imp->eventState == eMouseStateDraggingInnerTopRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(topRight.x(), topRight.y());
                        }
                        
                        
                        //////DRAWING OUTTER POINTS
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmLeft) || (_imp->eventState == eMouseStateDraggingOuterBtmLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchBtmLeft.x(), searchBtmLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidBtm.x(), outterMidBtm.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmRight) || (_imp->eventState == eMouseStateDraggingOuterBtmRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchBtmRight.x() , searchBtmRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidLeft.x(), outterMidLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidRight.x(), outterMidRight.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopLeft) || (_imp->eventState == eMouseStateDraggingOuterTopLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchTopLeft.x(), searchTopLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidTop.x(), outterMidTop.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopRight) || (_imp->eventState == eMouseStateDraggingOuterTopRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchTopRight.x(), searchTopRight.y());
                        }
                        
                        glEnd();
                        
                        if (offset.x() != 0 || offset.y() != 0) {
                            glBegin(GL_LINES);
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                            glVertex2d(center.x(), center.y());
                            glVertex2d(center.x() + offset.x(), center.y() + offset.y());
                            glEnd();
                        }
                        
                        ///now show small lines at handle positions
                        glBegin(GL_LINES);
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidLeft.x(), innerMidLeft.y());
                        glVertex2d(innerMidLeftExt.x(), innerMidLeftExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidTop.x(), innerMidTop.y());
                        glVertex2d(innerMidTopExt.x(), innerMidTopExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidRight.x(), innerMidRight.y());
                        glVertex2d(innerMidRightExt.x(), innerMidRightExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidBtm.x(), innerMidBtm.y());
                        glVertex2d(innerMidBtmExt.x(), innerMidBtmExt.y());
                        
                        //////DRAWING OUTTER HANDLES
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidLeft.x(), outterMidLeft.y());
                        glVertex2d(outterMidLeftExt.x(), outterMidLeftExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidTop.x(), outterMidTop.y());
                        glVertex2d(outterMidTopExt.x(), outterMidTopExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidRight.x(), outterMidRight.y());
                        glVertex2d(outterMidRightExt.x(), outterMidRightExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidBtm.x(), outterMidBtm.y());
                        glVertex2d(outterMidBtmExt.x(), outterMidBtmExt.y());
                        glEnd();
                        
                        glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                       
                        QColor c;
                        c.setRgbF(markerColor[0], markerColor[1], markerColor[2]);
                        viewer->renderText(center.x(), center.y(), name.c_str(), c, viewer->font());
                    }

                }
            }
        } // // if (_imp->panelv1) {
        
        
        if (_imp->clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5);
            //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
            for (int l = 0; l < 2; ++l) {
                // shadow (uses GL_PROJECTION)
                glMatrixMode(GL_PROJECTION);
                int direction = (l == 0) ? 1 : -1;
                // translate (1,-1) pixels
                glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                glMatrixMode(GL_MODELVIEW);
                
                if (l == 0) {
                    glColor4d(0., 0., 0., 0.8);
                } else {
                    glColor4d(0., 1., 0.,0.8);
                }
                
                glBegin(GL_LINE_LOOP);
                glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE * 2 * pixelScaleY);
                glEnd();
                
                ///draw a cross at the cursor position
                glBegin(GL_LINES);
                glVertex2d( _imp->lastMousePos.x() - ADDTRACK_SIZE * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d( _imp->lastMousePos.x() + ADDTRACK_SIZE * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() - ADDTRACK_SIZE * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() + ADDTRACK_SIZE * pixelScaleY);
                glEnd();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlays


static bool isNearbyPoint(const boost::shared_ptr<Double_Knob>& knob,
                          ViewerGL* viewer,
                          double xWidget, double yWidget,
                          double toleranceWidget,
                          double time)
{
    QPointF p;
    p.rx() = knob->getValueAtTime(time, 0);
    p.ry() = knob->getValueAtTime(time, 1);
    p = viewer->toWidgetCoordinates(p);
    if (p.x() <= (xWidget + toleranceWidget) && p.x() >= (xWidget - toleranceWidget) &&
        p.y() <= (yWidget + toleranceWidget) && p.y() >= (yWidget - toleranceWidget)) {
        return true;
    }
    return false;
}

static bool isNearbyPoint(const QPointF& p,
                          ViewerGL* viewer,
                          double xWidget, double yWidget,
                          double toleranceWidget)
{
    QPointF pw = viewer->toWidgetCoordinates(p);
    if (pw.x() <= (xWidget + toleranceWidget) && pw.x() >= (xWidget - toleranceWidget) &&
        pw.y() <= (yWidget + toleranceWidget) && pw.y() >= (yWidget - toleranceWidget)) {
        return true;
    }
    return false;
}

static void findLineIntersection(const Natron::Point& p, const Natron::Point& l1, const Natron::Point& l2, Natron::Point* inter)
{
    Natron::Point h,u;
    double a;
    h.x = p.x - l1.x;
    h.y = p.y - l1.y;
    
    u.x = l2.x - l1.x;
    u.y = l2.y - l1.y;
    
    a = (u.x * h.x + u.y * h.y) / (u.x * u.x + u.y * u.y);
    inter->x = l1.x + u.x * a;
    inter->y = l1.y + u.y * a;
}

bool
TrackerGui::penDown(double time,
                    double scaleX,
                    double scaleY,
                    const QPointF & viewportPos,
                    const QPointF & pos,
                    double pressure,
                    QMouseEvent* e)
{
    std::pair<double,double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
 
    
    if (_imp->panelv1) {
        
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenDown_public(time, scaleX, scaleY, viewportPos, pos, pressure);
            }
        }
        
        double selectionTol = pixelScale.first * 10.;
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x,y;
            x = dblKnob->getValueAtTime(time, 0);
            y = dblKnob->getValueAtTime(time, 1);
            
            if ( ( pos.x() >= (x - selectionTol) ) && ( pos.x() <= (x + selectionTol) ) &&
                ( pos.y() >= (y - selectionTol) ) && ( pos.y() <= (y + selectionTol) ) ) {
                if (!it->second) {
                    _imp->panelv1->selectNode( instance, modCASIsShift(e) );
                    
                }
                didSomething = true;
            }
        }
        
        if (_imp->clickToAddTrackEnabled && !didSomething) {
            boost::shared_ptr<Node> newInstance = _imp->panelv1->createNewInstance(true);
            boost::shared_ptr<KnobI> newInstanceKnob = newInstance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
            assert(dblKnob);
            dblKnob->beginChanges();
            dblKnob->blockValueChanges();
            dblKnob->setValueAtTime(time, pos.x(), 0);
            dblKnob->setValueAtTime(time, pos.y(), 1);
            dblKnob->unblockValueChanges();
            dblKnob->endChanges();
            didSomething = true;
        }
        
        if ( !didSomething && !modCASIsShift(e) ) {
            _imp->panelv1->clearSelection();
        }
    } else { // if (_imp->panelv1) {
        
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        context->getAllMarkers(&allMarkers);
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
            if (!(*it)->isEnabled()) {
                continue;
            }
            
            bool isSelected = context->isMarkerSelected((*it));
            
            boost::shared_ptr<Double_Knob> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<Double_Knob> offsetKnob = (*it)->getOffsetKnob();
            boost::shared_ptr<Double_Knob> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<Double_Knob> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<Double_Knob> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<Double_Knob> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            
            boost::shared_ptr<Double_Knob> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<Double_Knob> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            
            if (isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time)) {
                if (_imp->controlDown > 0) {
                    _imp->eventState = eMouseStateDraggingOffset;
                } else {
                    _imp->eventState = eMouseStateDraggingCenter;
                }
                _imp->interactMarker = *it;
                didSomething = true;
            }
            
            QPointF centerPoint;
            centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
            centerPoint.ry() = centerKnob->getValueAtTime(time, 1);
            
            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);
            if (!didSomething && isSelected) {
                
                QPointF topLeft,topRight,btmRight,btmLeft;
                topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                if (isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                }
            }
            if (!didSomething && isSelected) {
              
                
                ///Test search window
                QPointF searchTopLeft,searchTopRight,searchBtmRight,searchBtmLeft;
                searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
                
                searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + + centerPoint.y() + offset.y();
                
                searchTopLeft.rx() = searchBtmLeft.x();
                searchTopLeft.ry() = searchTopRight.y();
                
                searchBtmRight.rx() = searchTopRight.x();
                searchBtmRight.ry() = searchBtmLeft.y();
                
                if (isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                }
            }
            
            //If we hit the interact, make sure it is selected
            if (_imp->interactMarker) {
                if (!isSelected) {
                    context->addTrackToSelection(_imp->interactMarker, TrackerContext::eTrackSelectionViewer);
                }
            }
            
            if (didSomething) {
                break;
            }
        } // for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
        
        if (_imp->clickToAddTrackEnabled && !didSomething) {
            boost::shared_ptr<TrackMarker> marker = context->createMarker();
            boost::shared_ptr<Double_Knob> centerKnob = marker->getCenterKnob();
            centerKnob->setValuesAtTime(time, pos.x(), pos.y(), Natron::eValueChangedReasonNatronInternalEdited);
            didSomething = true;
        }
        
        if ( !didSomething && !modCASIsShift(e) ) {
            context->clearSelection(TrackerContext::eTrackSelectionViewer);
            didSomething = true;
        }
    }
    _imp->lastMousePos = pos;
    
    return didSomething;
} // penDown

bool
TrackerGui::penDoubleClicked(double /*time*/,
                             double /*scaleX*/,
                             double /*scaleY*/,
                             const QPointF & /*viewportPos*/,
                             const QPointF & /*pos*/,
                             QMouseEvent* /*e*/)
{
    bool didSomething = false;

    return didSomething;
}

bool
TrackerGui::penMotion(double time,
                      double scaleX,
                      double scaleY,
                      const QPointF & viewportPos,
                      const QPointF & pos,
                      double pressure,
                      QInputEvent* /*e*/)
{
    std::pair<double,double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    
    
    Natron::Point delta;
    delta.x = pos.x() - _imp->lastMousePos.x();
    delta.y = pos.y() - _imp->lastMousePos.y();
    
    if (_imp->panelv1) {
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                if ( effect->onOverlayPenMotion_public(time, scaleX, scaleY, viewportPos, pos, pressure) ) {
                    didSomething = true;
                }
            }
        }
    } else {
        
        if (_imp->hoverState != eDrawStateInactive) {
            _imp->hoverState = eDrawStateInactive;
            _imp->hoverMarker.reset();
            didSomething = true;
        }
        
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        context->getAllMarkers(&allMarkers);
        
        bool hoverProcess = false;
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
            if (!(*it)->isEnabled()) {
                continue;
            }
            
            bool isSelected = context->isMarkerSelected((*it));
            
            boost::shared_ptr<Double_Knob> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<Double_Knob> offsetKnob = (*it)->getOffsetKnob();
            
            boost::shared_ptr<Double_Knob> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<Double_Knob> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<Double_Knob> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<Double_Knob> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            
            boost::shared_ptr<Double_Knob> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<Double_Knob> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            
            if (isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time)) {
                _imp->hoverState = eDrawStateHoveringCenter;
                _imp->hoverMarker = *it;
                hoverProcess = true;
            }
            
            
            QPointF centerPoint;
            centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
            centerPoint.ry() = centerKnob->getValueAtTime(time, 1);
            
            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);
            
            if (!hoverProcess) {
                QPointF topLeft,topRight,btmRight,btmLeft;
                topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                if (isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                }
            }
            if (!hoverProcess && isSelected) {
               
                
                ///Test search window
                QPointF searchTopLeft,searchTopRight,searchBtmRight,searchBtmLeft;
                searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
                
                searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + + centerPoint.y() + offset.y();
                
                searchTopLeft.rx() = searchBtmLeft.x();
                searchTopLeft.ry() = searchTopRight.y();
                
                searchBtmRight.rx() = searchTopRight.x();
                searchBtmRight.ry() = searchBtmLeft.y();
                
                if (isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                }
            }
            
            if (hoverProcess) {
                break;
            }
        } // for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
        
        if (hoverProcess) {
            didSomething = true;
        }
        
        boost::shared_ptr<Double_Knob> centerKnob,offsetKnob,searchWndTopRight,searchWndBtmLeft;
        boost::shared_ptr<Double_Knob> patternCorners[4];
        if (_imp->interactMarker) {
            centerKnob = _imp->interactMarker->getCenterKnob();
            offsetKnob = _imp->interactMarker->getOffsetKnob();
            
            /*
             
             TopLeft(0) ------------- Top right(3)
             |                        |
             |                        |
             |                        |
             Btm left (1) ------------ Btm right (2)
             
             */
            patternCorners[0] = _imp->interactMarker->getPatternTopLeftKnob();
            patternCorners[1] = _imp->interactMarker->getPatternBtmLeftKnob();
            patternCorners[2] = _imp->interactMarker->getPatternBtmRightKnob();
            patternCorners[3] = _imp->interactMarker->getPatternTopRightKnob();
            searchWndTopRight = _imp->interactMarker->getSearchWindowTopRightKnob();
            searchWndBtmLeft = _imp->interactMarker->getSearchWindowBottomLeftKnob();
        }
        
        
        switch (_imp->eventState) {
            case eMouseStateDraggingCenter:
            case eMouseStateDraggingOffset:
            {
                assert(_imp->interactMarker);
                if (_imp->eventState == eMouseStateDraggingOffset) {
                    offsetKnob->setValues(offsetKnob->getValueAtTime(time,0) + delta.x,
                                           offsetKnob->getValueAtTime(time,1) + delta.y,
                                           Natron::eValueChangedReasonNatronInternalEdited);
                } else {
                    centerKnob->setValues(centerKnob->getValueAtTime(time,0) + delta.x,
                                          centerKnob->getValueAtTime(time,1) + delta.y,
                                          Natron::eValueChangedReasonNatronInternalEdited);
                }
                didSomething = true;
            }   break;
            case eMouseStateDraggingInnerBtmLeft:
            case eMouseStateDraggingInnerTopRight:
            case eMouseStateDraggingInnerTopLeft:
            case eMouseStateDraggingInnerBtmRight:
            {
                int index;
                if (_imp->eventState == eMouseStateDraggingInnerBtmLeft) {
                    index = 1;
                } else if (_imp->eventState == eMouseStateDraggingInnerBtmRight) {
                    index = 2;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopRight) {
                    index = 3;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopLeft) {
                    index = 0;
                }
                
                int nextIndex = (index + 1) % 4;
                int prevIndex = (index + 3) % 4;
                int diagIndex = (index + 2) % 4;
                
                
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point cur,prev,next,diag;
                cur.x = patternCorners[index]->getValueAtTime(time,0) + delta.x  + center.x + offset.x;;
                cur.y = patternCorners[index]->getValueAtTime(time,1) + delta.y  + center.y + offset.y;
                
                prev.x = patternCorners[prevIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                prev.y = patternCorners[prevIndex]->getValueAtTime(time,1) + center.y + offset.y;
                
                next.x = patternCorners[nextIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                next.y = patternCorners[nextIndex]->getValueAtTime(time,1)  + center.y + offset.y;
                
                diag.x = patternCorners[diagIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                diag.y = patternCorners[diagIndex]->getValueAtTime(time,1) + center.y + offset.y;
                
                Natron::Point nextVec;
                nextVec.x = next.x - cur.x;
                nextVec.y = next.y - cur.y;
                
                Natron::Point prevVec;
                prevVec.x = cur.x - prev.x;
                prevVec.y = cur.y - prev.y;
                
                Natron::Point nextDiagVec,prevDiagVec;
                nextDiagVec.x = diag.x - next.x;
                nextDiagVec.y = diag.y - next.y;
                
                prevDiagVec.x = prev.x - diag.x;
                prevDiagVec.y = prev.y - diag.y;
                
                //Clamp so the 4 points remaing the same in the homography
                if (prevVec.x * nextVec.y - prevVec.y * nextVec.x < 0.) { // cross-product
                    findLineIntersection(cur, prev, next, &cur);
                }
                if (nextDiagVec.x * prev.y - nextDiagVec.y * prev.x < 0.) { // cross-product
                    findLineIntersection(cur, prev, diag, &cur);
                }
                if (next.x * prevDiagVec.y - next.y * prevDiagVec.x < 0.) { // cross-product
                    findLineIntersection(cur, next, diag, &cur);
                }
                
                
                Natron::Point searchWindowCorners[4];
                searchWindowCorners[1].x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
                searchWindowCorners[1].y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
                
                searchWindowCorners[3].x = searchWndTopRight->getValueAtTime(time, 0)  + center.x + offset.x;;
                searchWindowCorners[3].y = searchWndTopRight->getValueAtTime(time, 1)  + center.y + offset.y;;
                
                cur.x = std::max(std::min(cur.x, searchWindowCorners[3].x), searchWindowCorners[1].x);
                cur.y = std::max(std::min(cur.y, searchWindowCorners[3].y), searchWindowCorners[1].y);
                
                if (patternCorners[index]->hasAnimation()) {
                    patternCorners[index]->setValuesAtTime(time, cur.x, cur.y, Natron::eValueChangedReasonNatronInternalEdited);
                } else {
                    patternCorners[index]->setValues(cur.x, cur.y, Natron::eValueChangedReasonNatronInternalEdited);
                }
                
                didSomething = true;
            }   break;
            case eMouseStateDraggingOuterBtmLeft:
            case eMouseStateDraggingOuterBtmRight:
            case eMouseStateDraggingOuterTopLeft:
            case eMouseStateDraggingOuterTopRight:
            {
                
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                
                Natron::Point searchWindowCorners[4];
                searchWindowCorners[1].x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
                searchWindowCorners[1].y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
                
                searchWindowCorners[3].x = searchWndTopRight->getValueAtTime(time, 0)  + center.x + offset.x;;
                searchWindowCorners[3].y = searchWndTopRight->getValueAtTime(time, 1)  + center.y + offset.y;;
                
                searchWindowCorners[0].x = searchWindowCorners[1].x;
                searchWindowCorners[0].y = searchWindowCorners[3].y;
                
                searchWindowCorners[2].x = searchWindowCorners[3].x;
                searchWindowCorners[2].y = searchWindowCorners[1].y;
                
                int index;
                if (_imp->eventState == eMouseStateDraggingOuterBtmLeft) {
                    index = 1;
                } else if (_imp->eventState == eMouseStateDraggingOuterBtmRight) {
                    index = 2;
                } else if (_imp->eventState == eMouseStateDraggingOuterTopLeft) {
                    index = 0;
                } else if (_imp->eventState == eMouseStateDraggingOuterTopRight) {
                    index = 3;
                }
                
                
            }   break;
            default:
                break;
        }

    }
    if (_imp->clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    _imp->lastMousePos = pos;

    return didSomething;
}

bool
TrackerGui::penUp(double time,
                  double scaleX,
                  double scaleY,
                  const QPointF & viewportPos,
                  const QPointF & pos,
                  double pressure,
                  QMouseEvent* /*e*/)
{
    bool didSomething = false;
    
    TrackerMouseStateEnum state = _imp->eventState;
    _imp->eventState = eMouseStateIdle;
    if (_imp->panelv1) {
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenUp_public(time, scaleX, scaleY, viewportPos, pos, pressure);
                if (didSomething) {
                    return true;
                }
            }
        }
    } else { // if (_imp->panelv1) {
        _imp->interactMarker.reset();
        (void)state;
    } // if (_imp->panelv1) {

    return didSomething;
}

bool
TrackerGui::keyDown(double time,
                    double scaleX,
                    double scaleY,
                    QKeyEvent* e)
{
    bool didSomething = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();


    if (e->key() == Qt::Key_Control) {
        ++_imp->controlDown;
    }

    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    if (_imp->panelv1) {
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyDown_public(time, scaleX, scaleY, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }
            }
        }
    }
    
    if ( modCASIsControlAlt(e) && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = true;
        _imp->addTrackButton->setDown(true);
        _imp->addTrackButton->setChecked(true);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onSelectAllButtonClicked();
            std::list<Natron::Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onDeleteKeyPressed();
            std::list<Natron::Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, modifiers, key) ) {
        _imp->trackBwButton->setDown(true);
        _imp->trackBwButton->setChecked(true);
        if (_imp->panelv1) {
            didSomething = _imp->panelv1->trackBackward();
            if (!didSomething) {
                _imp->panelv1->stopTracking();
                _imp->trackBwButton->setDown(false);
                _imp->trackBwButton->setChecked(false);
            }
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, modifiers, key) ) {
        if (_imp->panelv1) {
            didSomething = _imp->panelv1->trackPrevious();
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, modifiers, key) ) {
        if (_imp->panelv1) {
            didSomething = _imp->panelv1->trackNext();
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, modifiers, key) ) {
        _imp->trackFwButton->setDown(true);
        _imp->trackFwButton->setChecked(true);
        if (_imp->panelv1) {
            didSomething = _imp->panelv1->trackForward();
            if (!didSomething) {
                _imp->panelv1->stopTracking();
                _imp->trackFwButton->setDown(false);
                _imp->trackFwButton->setChecked(false);
            }
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->stopTracking();
        }
    }

    return didSomething;
} // keyDown

bool
TrackerGui::keyUp(double time,
                  double scaleX,
                  double scaleY,
                  QKeyEvent* e)
{
    bool didSomething = false;

    if (e->key() == Qt::Key_Control) {
        if (_imp->controlDown > 0) {
            --_imp->controlDown;
        }
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    if (_imp->panelv1) {
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyUp_public(time, scaleX, scaleY, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }
            }
        }
    }
    if ( _imp->clickToAddTrackEnabled && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = false;
        _imp->addTrackButton->setDown(false);
        _imp->addTrackButton->setChecked(false);
        didSomething = true;
    }

    return didSomething;
}

bool
TrackerGui::loseFocus(double time,
                      double scaleX,
                      double scaleY)
{
    bool didSomething = false;
    
    _imp->controlDown = 0;
    
    if (_imp->panelv1) {
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething |= effect->onOverlayFocusLost_public(time, scaleX, scaleY);
            }
        }
    }

    return didSomething;
}

void
TrackerGui::updateSelectionFromSelectionRectangle(bool onRelease)
{
    if (!onRelease) {
        return;
    }
    double l,r,b,t;
    _imp->viewer->getViewer()->getSelectionRectangle(l, r, b, t);
    
    if (_imp->panelv1) {
        std::list<Natron::Node*> currentSelection;
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Node> instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x,y;
            x = dblKnob->getValue(0);
            y = dblKnob->getValue(1);
            if ( (x >= l) && (x <= r) && (y >= b) && (y <= t) ) {
                ///assert that the node is really not part of the selection
                assert( std::find( currentSelection.begin(),currentSelection.end(),instance.get() ) == currentSelection.end() );
                currentSelection.push_back( instance.get() );
            }
        }
        _imp->panelv1->selectNodes( currentSelection, (_imp->controlDown > 0) );
    }
}

void
TrackerGui::onSelectionCleared()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearSelection();
    }
}

void
TrackerGui::onTrackBwClicked()
{
    _imp->trackBwButton->setDown(true);
    if (!_imp->panelv1->trackBackward()) {
        _imp->panelv1->stopTracking();
        _imp->trackBwButton->setDown(false);
        _imp->trackBwButton->setChecked(false);
    }
}

void
TrackerGui::onTrackPrevClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->trackPrevious();
    }
}

void
TrackerGui::onStopButtonClicked()
{
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
    if (_imp->panelv1) {
        _imp->panelv1->stopTracking();
    }
}

void
TrackerGui::onTrackNextClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->trackNext();
    }
}

void
TrackerGui::onTrackFwClicked()
{
    _imp->trackFwButton->setDown(true);
    if (_imp->panelv1) {
        if (!_imp->panelv1->trackForward()) {
            _imp->panelv1->stopTracking();
            _imp->trackFwButton->setDown(false);
            _imp->trackFwButton->setChecked(false);
        }
    }
}

void
TrackerGui::onUpdateViewerClicked(bool clicked)
{
    if (_imp->panelv1) {
        _imp->panelv1->setUpdateViewerOnTracking(clicked);
    }
    _imp->updateViewerButton->setDown(clicked);
    _imp->updateViewerButton->setChecked(clicked);
}

void
TrackerGui::onTrackingEnded()
{
    _imp->trackBwButton->setChecked(false);
    _imp->trackFwButton->setChecked(false);
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
}

void
TrackerGui::onClearAllAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearAllAnimationForSelection();
    }
}

void
TrackerGui::onClearBwAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearBackwardAnimationForSelection();
    }
}

void
TrackerGui::onClearFwAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearForwardAnimationForSelection();
    }
}

void
TrackerGui::onCreateKeyOnMoveButtonClicked(bool clicked)
{
    _imp->createKeyOnMoveButton->setDown(clicked);
}

void
TrackerGui::onShowCorrelationButtonClicked(bool clicked)
{
    _imp->showCorrelationButton->setDown(clicked);
}

void
TrackerGui::onCenterViewerButtonClicked(bool clicked)
{
    _imp->centerViewerButton->setDown(clicked);
}

void
TrackerGui::onSetKeyframeButtonClicked()
{
    int time = _imp->panel->getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->setUserKeyframe(time);
    }
}

void
TrackerGui::onRemoveKeyframeButtonClicked()
{
    int time = _imp->panel->getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeUserKeyframe(time);
    }
}

void
TrackerGui::onRemoveAnimationButtonClicked()
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeAllKeyframes();
    }
}

void
TrackerGui::onResetOffsetButtonClicked()
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->resetOffset();
    }
}

void
TrackerGui::onResetTrackButtonClicked()
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->resetTrack();
    }
}