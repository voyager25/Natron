//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef TRACKERCONTEXT_H
#define TRACKERCONTEXT_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <set>

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QThread>
#include <QMutex>

namespace Natron {
    class Node;
}
class Double_Knob;
class Bool_Knob;
class Button_Knob;
class Choice_Knob;
class KnobI;
class TimeLine;
class TrackerContext;
struct TrackMarkerPrivate;
class TrackMarker
{
public:
    
    TrackMarker(const boost::shared_ptr<TrackerContext>& context);
    
    ~TrackMarker();
    
    boost::shared_ptr<TrackerContext> getContext() const;
    
    bool setScriptName(const std::string& name);
    std::string getScriptName() const;
    
    void setLabel(const std::string& label);
    std::string getLabel() const;
    
    boost::shared_ptr<Double_Knob> getSearchWindowBottomLeftKnob() const;
    boost::shared_ptr<Double_Knob> getSearchWindowTopRightKnob() const;
    boost::shared_ptr<Double_Knob> getPatternTopLeftKnob() const;
    boost::shared_ptr<Double_Knob> getPatternTopRightKnob() const;
    boost::shared_ptr<Double_Knob> getPatternBtmRightKnob() const;
    boost::shared_ptr<Double_Knob> getPatternBtmLeftKnob() const;
    boost::shared_ptr<Double_Knob> getWeightKnob() const;
    boost::shared_ptr<Double_Knob> getCenterKnob() const;
    boost::shared_ptr<Double_Knob> getOffsetKnob() const;
    boost::shared_ptr<Double_Knob> getCorrelationKnob() const;
    boost::shared_ptr<Choice_Knob> getMotionModelKnob() const;
    
    const std::list<boost::shared_ptr<KnobI> >& getKnobs() const;
    
    int getReferenceFrame(int time, bool forward) const;
    
    bool isUserKeyframe(int time) const;
    
    void getUserKeyframes(std::set<int>* keyframes) const;
    
    bool isEnabled() const;
    
    void setEnabled(bool enabled);
    
private:
    
    boost::scoped_ptr<TrackMarkerPrivate> _imp;
    
};

struct TrackerContextPrivate;
class TrackerContext : public QObject, public boost::enable_shared_from_this<TrackerContext>
{
    Q_OBJECT
    
public:
    
    enum TrackSelectionReason
    {
        eTrackSelectionSettingsPanel,
        eTrackSelectionViewer,
        eTrackSelectionInternal
    };
    
    TrackerContext(const boost::shared_ptr<Natron::Node> &node);
    
    ~TrackerContext();
    
    boost::shared_ptr<Natron::Node> getNode() const;
    
    boost::shared_ptr<TrackMarker> createMarker();
    
    void removeMarker(const TrackMarker* marker);
    
    boost::shared_ptr<TrackMarker> getMarkerByName(const std::string & name) const;
    
    std::string generateUniqueTrackName(const std::string& baseName);
    
    int getTimeLineFirstFrame() const;
    int getTimeLineLastFrame() const;
    
    /**
     * @brief Tracks the selected markers over the range defined by [start,end[ (end pointing to the frame
     * after the last one, a la STL).
     **/
    void trackSelectedMarkers(int start, int end, bool forward, bool updateViewer);
    
    void beginEditSelection();
    
    void endEditSelection(TrackSelectionReason reason);
    
    void addTracksToSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason);
    void addTrackToSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason);
    
    void removeTracksFromSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason);
    void removeTrackFromSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason);
    
    void clearSelection(TrackSelectionReason reason);
    
    void getSelectedMarkers(std::list<boost::shared_ptr<TrackMarker> >* markers) const;
    
public Q_SLOTS:
    
    void onSelectedKnobCurveChanged();
    
    
Q_SIGNALS:
    
    //reason is of type TrackSelectionReason
    void selectionChanged(int reason);
    
private:
    
    void endSelection(TrackSelectionReason reason);
    
    boost::scoped_ptr<TrackerContextPrivate> _imp;
};

class TrackSchedulerBase : public QThread
{
    Q_OBJECT
    
public:
    
    TrackSchedulerBase() : QThread() {}
    
    virtual ~TrackSchedulerBase() {}
    
protected:
    
    void emit_trackingStarted()
    {
        Q_EMIT trackingStarted();
    }
    
    void emit_trackingFinished()
    {
        Q_EMIT trackingFinished();
    }
    
    void emit_progressUpdate(double p)
    {
        Q_EMIT progressUpdate(p);
    }
    
Q_SIGNALS:
    
    void trackingStarted();
    
    void trackingFinished();
    
    void progressUpdate(double progress);
    
    
};


struct TrackSchedulerPrivate;
template <class TrackArgsType>
class TrackScheduler : public TrackSchedulerBase
{
    
public:
    
    /*
     * @brief A pointer to a function that will be called concurrently for each Track marker to track.
     * @param index Identifies the track in args, which is supposed to hold the tracks vector.
     * @param time The time at which to track. The reference frame is held in the args and can be different for each track
     */
    typedef bool (*TrackStepFunctor)(int trackIndex, const TrackArgsType& args, int time);
    
    TrackScheduler(TrackStepFunctor functor);
    
    virtual ~TrackScheduler();
    
    /**
     * @brief Track the selectedInstances, calling the instance change action on each button (either the previous or
     * next button) in a separate thread.
     * @param start the first frame to track, if forward is true then start < end
     * @param end the next frame after the last frame to track (a la STL iterators), if forward is true then end > start
     **/
    void track(const TrackArgsType& args);
    
    void abortTracking();
    
    void quitThread();
    
    bool isWorking() const;
    

    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<TrackSchedulerPrivate> _imp;
    
    QMutex argsMutex;
    TrackArgsType curArgs,requestedArgs;
    TrackStepFunctor _functor;
};

#endif // TRACKERCONTEXT_H
