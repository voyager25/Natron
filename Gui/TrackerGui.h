//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef TRACKERGUI_H
#define TRACKERGUI_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include "Global/GLobalDefines.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class QWidget;
class ViewerTab;
class TrackerPanelV1;
class TrackerPanel;
class QKeyEvent;
class QPointF;
class QMouseEvent;
class QInputEvent;
class TrackMarker;

struct TrackerGuiPrivate;
class TrackerGui
    : public QObject
{
    Q_OBJECT

public:

    TrackerGui(const boost::shared_ptr<TrackerPanelV1> & panel,
               ViewerTab* parent);
    
    TrackerGui(TrackerPanel* panel,
               ViewerTab* parent);

    virtual ~TrackerGui();

    /**
     * @brief Return the horizontal buttons bar.
     **/
    QWidget* getButtonsBar() const;


    void drawOverlays(double time, double scaleX, double scaleY) const;

    bool penDown(double time, double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure, QMouseEvent* e);

    bool penDoubleClicked(double time, double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, QMouseEvent* e);

    bool penMotion(double time, double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure, QInputEvent* e);

    bool penUp(double time, double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure, QMouseEvent* e);

    bool keyDown(double time, double scaleX, double scaleY, QKeyEvent* e);

    bool keyUp(double time, double scaleX, double scaleY, QKeyEvent* e);

    bool loseFocus(double time, double scaleX, double scaleY);

public Q_SLOTS:

    void onTimelineTimeChanged(SequenceTime time, int reason);
    
    void onAddTrackClicked(bool clicked);

    void onTrackBwClicked();

    void onTrackPrevClicked();

    void onStopButtonClicked();

    void onTrackNextClicked();

    void onTrackFwClicked();

    void onUpdateViewerClicked(bool clicked);

    void onClearAllAnimationClicked();

    void onClearBwAnimationClicked();

    void onClearFwAnimationClicked();

    void updateSelectionFromSelectionRectangle(bool onRelease);

    void onSelectionCleared();

    void onTrackingEnded();
    
    void onCreateKeyOnMoveButtonClicked(bool clicked);
    
    void onShowCorrelationButtonClicked(bool clicked);
    
    void onCenterViewerButtonClicked(bool clicked);
    
    void onSetKeyframeButtonClicked();
    void onRemoveKeyframeButtonClicked();
    void onResetOffsetButtonClicked();
    void onResetTrackButtonClicked();
    
    void onContextSelectionChanged(int reason);
    void onKeyframeSetOnTrack(const boost::shared_ptr<TrackMarker> &marker, int key);
    void onKeyframeRemovedOnTrack(const boost::shared_ptr<TrackMarker> &marker, int key);
    void onAllKeyframesRemovedOnTrack(const boost::shared_ptr<TrackMarker>& marker);
 
    void updateSelectedMarkerTexture();
    
private Q_SLOTS:
    void onTrackImageRenderingFinished();
    
private:
    
    void createGui();

    boost::scoped_ptr<TrackerGuiPrivate> _imp;
};

#endif // TRACKERGUI_H
