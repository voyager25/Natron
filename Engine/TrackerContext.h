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
class TrackSerialization;
class TrackerContextSerialization;

struct TrackMarkerPrivate;
class TrackMarker : QObject, public boost::enable_shared_from_this<TrackMarker>
{
    Q_OBJECT
    
public:
    
    TrackMarker(const boost::shared_ptr<TrackerContext>& context);
    
    ~TrackMarker();
    
    void clone(const TrackMarker& other);
    
    void load(const TrackSerialization& serialization);
    
    void save(TrackSerialization* serialization) const;
    
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
    
    int getPreviousKeyframe(int time) const;
    
    int getNextKeyframe( int time) const;
    
    void getUserKeyframes(std::set<int>* keyframes) const;
    
    void getCenterKeyframes(std::set<int>* keyframes) const;
    
    bool isEnabled() const;
    
    void setEnabled(bool enabled, int reason);
    
    void resetCenter();
    
    void resetOffset();
    
    void resetTrack();
    
    void setUserKeyframe(int time);
    
    void removeUserKeyframe(int time);
    
    void removeAllKeyframes();
    
    
public Q_SLOTS:
    
    void onCenterKeyframeSet(SequenceTime time,int dimension,int reason,bool added);
    void onCenterKeyframeRemoved(SequenceTime time,int dimension,int reason);
    void onCenterKeyframeMoved(int dimension,int oldTime,int newTime);
    void onCenterKeyframesSet(const std::list<SequenceTime>& keys, int dimension, int reason);
    void onCenterAnimationRemoved(int dimension);
    
    void onCenterKnobValueChanged(int dimension,int reason);
    void onOffsetKnobValueChanged(int dimension,int reason);
    void onCorrelationKnobValueChanged(int dimension,int reason);
    void onWeightKnobValueChanged(int dimension,int reason);
    void onMotionModelKnobValueChanged(int dimension,int reason);
    
    /*void onPatternTopLeftKnobValueChanged(int dimension,int reason);
    void onPatternTopRightKnobValueChanged(int dimension,int reason);
    void onPatternBtmRightKnobValueChanged(int dimension,int reason);
    void onPatternBtmLeftKnobValueChanged(int dimension,int reason);
    
    void onSearchBtmLeftKnobValueChanged(int dimension,int reason);
    void onSearchTopRightKnobValueChanged(int dimension,int reason);*/
    
private:
    
    boost::scoped_ptr<TrackMarkerPrivate> _imp;
    
};


class TrackArgsV1
{
    int _start,_end;
    bool _forward;
    boost::shared_ptr<TimeLine> _timeline;
    std::vector<Button_Knob*> _buttonInstances;
    bool _isUpdateViewerEnabled;
    
    
public:
    
    TrackArgsV1()
    : _start(0)
    , _end(0)
    , _forward(false)
    , _timeline()
    , _buttonInstances()
    , _isUpdateViewerEnabled(false)
    {
        
    }
    
    TrackArgsV1(const TrackArgsV1& other)
    {
        *this = other;
    }
    
    TrackArgsV1(int start,
                int end,
                bool forward,
                const boost::shared_ptr<TimeLine>& timeline,
                const std::vector<Button_Knob*>& instances,
                bool updateViewer)
    : _start(start)
    , _end(end)
    , _forward(forward)
    , _timeline(timeline)
    , _buttonInstances(instances)
    , _isUpdateViewerEnabled(updateViewer)
    {
        
    }
    
    ~TrackArgsV1() {}
    
    void operator=(const TrackArgsV1& other)
    {
        _start = other._start;
        _end = other._end;
        _forward = other._forward;
        _timeline = other._timeline;
        _buttonInstances = other._buttonInstances;
        _isUpdateViewerEnabled = other._isUpdateViewerEnabled;
    }
    
    bool isUpdateViewerEnabled() const
    {
        return _isUpdateViewerEnabled;
    }
    
    int getStart() const
    {
        return _start;
    }
    
    int getEnd() const
    {
        return _end;
    }
    
    bool getForward() const
    {
        return _forward;
    }
    
    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }
    
    const std::vector<Button_Knob*>& getInstances() const
    {
        return _buttonInstances;
    }
    
    int getNumTracks() const
    {
        return (int)_buttonInstances.size();
    }
    
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
        eTrackSelectionInternal,
    };
    
    TrackerContext(const boost::shared_ptr<Natron::Node> &node);
    
    ~TrackerContext();
    
    void load(const TrackerContextSerialization& serialization);
    
    void save(TrackerContextSerialization* serialization) const;

    
    boost::shared_ptr<Natron::Node> getNode() const;
    
    boost::shared_ptr<TrackMarker> createMarker();
    
    int getMarkerIndex(const boost::shared_ptr<TrackMarker>& marker) const;
    
    boost::shared_ptr<TrackMarker> getPrevMarker(const boost::shared_ptr<TrackMarker>& marker, bool loop) const;
    boost::shared_ptr<TrackMarker> getNextMarker(const boost::shared_ptr<TrackMarker>& marker, bool loop) const;
    
    void appendMarker(const boost::shared_ptr<TrackMarker>& marker);
    
    void insertMarker(const boost::shared_ptr<TrackMarker>& marker, int index);
    
    void removeMarker(const boost::shared_ptr<TrackMarker>& marker);
    
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
    
    void selectAll(TrackSelectionReason reason);
    
    void getAllMarkers(std::vector<boost::shared_ptr<TrackMarker> >* markers) const;
    
    void getSelectedMarkers(std::list<boost::shared_ptr<TrackMarker> >* markers) const;
    
    bool isMarkerSelected(const boost::shared_ptr<TrackMarker>& marker) const;
    
    static void getMotionModelsAndHelps(std::vector<std::string>* models,std::vector<std::string>* tooltips);
    
    int getTransformReferenceFrame() const;
    
    void goToPreviousKeyFrame(int time);
    void goToNextKeyFrame(int time);
    
    static bool trackStepV1(int trackIndex, const TrackArgsV1& args, int time);

    
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
    
    void s_keyframeSetOnTrack(const boost::shared_ptr<TrackMarker>& marker,int key) { Q_EMIT keyframeSetOnTrack(marker,key); }
    void s_keyframeRemovedOnTrack(const boost::shared_ptr<TrackMarker>& marker,int key) { Q_EMIT keyframeRemovedOnTrack(marker,key); }
    void s_allKeyframesRemovedOnTrack(const boost::shared_ptr<TrackMarker>& marker) { Q_EMIT allKeyframesRemovedOnTrack(marker); }
    
    void s_keyframeSetOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker,int key) { Q_EMIT keyframeSetOnTrackCenter(marker,key); }
    void s_keyframeRemovedOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker,int key) { Q_EMIT keyframeRemovedOnTrackCenter(marker,key); }
    void s_allKeyframesRemovedOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker) { Q_EMIT allKeyframesRemovedOnTrackCenter(marker); }
    void s_multipleKeyframesSetOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker, const std::list<int>& keys) { Q_EMIT multipleKeyframesSetOnTrackCenter(marker,keys); }
    
    void s_trackAboutToClone(const boost::shared_ptr<TrackMarker>& marker) { Q_EMIT trackAboutToClone(marker); }
    void s_trackCloned(const boost::shared_ptr<TrackMarker>& marker) { Q_EMIT trackCloned(marker); }
    
    void s_enabledChanged(boost::shared_ptr<TrackMarker> marker,int reason) { Q_EMIT enabledChanged(marker, reason); }
    
    void s_centerKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT centerKnobValueChanged(marker,dimension,reason); }
    void s_offsetKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT offsetKnobValueChanged(marker,dimension,reason); }
    void s_correlationKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT correlationKnobValueChanged(marker,dimension,reason); }
    void s_weightKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT weightKnobValueChanged(marker,dimension,reason); }
    void s_motionModelKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT motionModelKnobValueChanged(marker,dimension,reason); }
    
   /* void s_patternTopLeftKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT patternTopLeftKnobValueChanged(marker,dimension,reason); }
    void s_patternTopRightKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT patternTopRightKnobValueChanged(marker,dimension, reason); }
    void s_patternBtmRightKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT patternBtmRightKnobValueChanged(marker,dimension, reason); }
    void s_patternBtmLeftKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT patternBtmLeftKnobValueChanged(marker,dimension, reason); }
    
    void s_searchBtmLeftKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT searchBtmLeftKnobValueChanged(marker,dimension,reason); }
    void s_searchTopRightKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension,int reason) { Q_EMIT searchTopRightKnobValueChanged(marker, dimension, reason); }*/
    
public Q_SLOTS:
    
    void onSelectedKnobCurveChanged();
    
    
Q_SIGNALS:
    
    void keyframeSetOnTrack(boost::shared_ptr<TrackMarker> marker, int);
    void keyframeRemovedOnTrack(boost::shared_ptr<TrackMarker> marker, int);
    void allKeyframesRemovedOnTrack(boost::shared_ptr<TrackMarker> marker);
    
    void keyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int);
    void keyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int);
    void allKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker);
    void multipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, std::list<int>);
    
    void trackAboutToClone(boost::shared_ptr<TrackMarker> marker);
    void trackCloned(boost::shared_ptr<TrackMarker> marker);
    
    //reason is of type TrackSelectionReason
    void selectionChanged(int reason);
    void selectionAboutToChange(int reason);
    
    void trackInserted(boost::shared_ptr<TrackMarker> marker,int index);
    void trackRemoved(boost::shared_ptr<TrackMarker> marker);
    
    void enabledChanged(boost::shared_ptr<TrackMarker> marker,int reason);
    
    void centerKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void offsetKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void correlationKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void weightKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void motionModelKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    
    /*void patternTopLeftKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void patternTopRightKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void patternBtmRightKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void patternBtmLeftKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    
    void searchBtmLeftKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);
    void searchTopRightKnobValueChanged(boost::shared_ptr<TrackMarker> marker,int,int);*/
    
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
