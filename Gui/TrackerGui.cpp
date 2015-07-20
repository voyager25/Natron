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
#define ADDTRACK_SIZE 5

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

    
    _imp->centerViewerButton = new Button(QIcon(),"Center viewer",_imp->buttonsBar);
    _imp->centerViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->centerViewerButton->setCheckable(true);
    _imp->centerViewerButton->setChecked(false);
    _imp->centerViewerButton->setDown(false);
    _imp->centerViewerButton->setToolTip(Natron::convertFromPlainText(tr("Center the viewer on selected tracks during tracking."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->centerViewerButton,SIGNAL( clicked(bool) ),this,SLOT( onCenterViewerButtonClicked(bool) ) );
    _imp->buttonsLayout->addWidget(_imp->centerViewerButton);

    
    if (_imp->panel) {
        /// This is for v2 only
        _imp->createKeyOnMoveButton = new Button(QIcon(), "+ Key on move", _imp->buttonsBar);
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
        
        QPixmap addKeyPix,removeKeyPix,resetOffsetPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_USER_KEY, &addKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_USER_KEY, &removeKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESET_TRACK_OFFSET, &resetOffsetPix);
        
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
        
        _imp->removeAllKeyFramesButton = new Button(QIcon(), "--", keyframeContainer);
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

void
TrackerGui::drawOverlays(double time,
                         double scaleX,
                         double scaleY) const
{
    double pixelScaleX, pixelScaleY;
    _imp->viewer->getViewer()->getPixelScale(pixelScaleX, pixelScaleY);
    
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
        } // if (_imp->panelv1) {
        else {
            assert(_imp->panel);
            double markerColor[3];
            _imp->panel->getNode()->getOverlayColor(&markerColor[0], &markerColor[1], &markerColor[2]);
            
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
                
                if (!isSelected) {
                    ///Draw a custom interact, indicating the track isn't selected
                    boost::shared_ptr<Double_Knob> center = (*it)->getCenterKnob();
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
                        
                        
                        double x = center->getValueAtTime(time,0);
                        double y = center->getValueAtTime(time,1);
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
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlays

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
    _imp->viewer->getViewer()->getPixelScale(pixelScale.first, pixelScale.second);
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
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
                
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
    bool didSomething = false;
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
    }

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