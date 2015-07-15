//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "TrackerContext.h"

#include <set>
#include <sstream>

#include <QWaitCondition>
#include <QThread>
#include <QCoreApplication>
#include <QtConcurrentMap>

#include <libmv/autotrack/autotrack.h>
#include <libmv/autotrack/frame_accessor.h>
#include <libmv/image/array_nd.h>

#include <boost/bind.hpp>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"

#define kTrackBaseName "track"


/// Parameters definitions

//////// Global to all tracks
#define kTrackerParamTrackRed "trackRed"
#define kTrackerParamTrackRedLabel "Track Red"
#define kTrackerParamTrackRedHint "Enable tracking on the red channel"

#define kTrackerParamTrackGreen "trackGreen"
#define kTrackerParamTrackGreenLabel "Track Green"
#define kTrackerParamTrackGreenHint "Enable tracking on the green channel"

#define kTrackerParamTrackBlue "trackBlue"
#define kTrackerParamTrackBlueLabel "Track Blue"
#define kTrackerParamTrackBlueHint "Enable tracking on the blue channel"

#define kTrackerParamMinimumCorrelation "minCorrelation"
#define kTrackerParamMinimumCorrelationLabel "Minimum correlation"
#define kTrackerParamMinimumCorrelationHint "Minimum normalized cross-correlation necessary between the final tracked " \
"position of the patch on the destination image and the reference patch needed to declare tracking success."

#define kTrackerParamMaximumIteration "maxIterations"
#define kTrackerParamMaximumIterationLabel "Maximum iterations"
#define kTrackerParamMaximumIterationHint "Maximum number of iterations the algorithm will run for the inner minimization " \
"before it gives up."

#define kTrackerParamBruteForcePreTrack "bruteForcePretTrack"
#define kTrackerParamBruteForcePreTrackLabel "Use brute-force pre-track"
#define kTrackerParamBruteForcePreTrackHint "Use a brute-force translation-only pre-track before refinement"

#define kTrackerParamNormalizeIntensities "normalizeIntensities"
#define kTrackerParamNormalizeIntensitiesLabel "Normalize Intensities"
#define kTrackerParamNormalizeIntensitiesHint "Normalize the image patches by their mean before doing the sum of squared" \
" error calculation. Slower."

#define kTrackerParamPreBlurSigma "preBlurSigma"
#define kTrackerParamPreBlurSigmaLabel "Pre-blur sigma"
#define kTrackerParamPreBlurSigmaHint "The size in pixels of the blur kernel used to both smooth the image and take the image derivative."



//////// Per-track parameters
#define kTrackerParamSearchWndBtmLeft "searchWndBtmLeft"
#define kTrackerParamSearchWndBtmLeftLabel "Search Window Bottom Left"
#define kTrackerParamSearchWndBtmLeftHint "The bottom left point of the search window, relative to the center point."

#define kTrackerParamSearchWndTopRight "searchWndTopRight"
#define kTrackerParamSearchWndTopRightLabel "Search Window Top Right"
#define kTrackerParamSearchWndTopRightHint "The top right point of the search window, relative to the center point."

#define kTrackerParamPatternTopLeft "patternTopLeft"
#define kTrackerParamPatternTopLeftLabel "Pattern Top Left"
#define kTrackerParamPatternTopLeftHint "The top left point of the quad defining the pattern to track"

#define kTrackerParamPatternTopRight "patternTopRight"
#define kTrackerParamPatternTopRightLabel "Pattern Top Right"
#define kTrackerParamPatternTopRightHint "The top right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmRight "patternBtmRight"
#define kTrackerParamPatternBtmRightLabel "Pattern Bottom Right"
#define kTrackerParamPatternBtmRightHint "The bottom right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmLeft "patternBtmLeft"
#define kTrackerParamPatternBtmLeftLabel "Pattern Bottom Left"
#define kTrackerParamPatternBtmLeftHint "The bottom left point of the quad defining the pattern to track"

#define kTrackerParamCenter "centerPoint"
#define kTrackerParamCenterLabel "Center"
#define kTrackerParamCenterHint "The point to track"

#define kTrackerParamOffset "offset"
#define kTrackerParamOffsetLabel "Offset"
#define kTrackerParamOffsetHint "The offset applied to the center point relative to the real tracked position"

#define kTrackerParamTrackWeight "trackWeight"
#define kTrackerParamTrackWeightLabel "Weight"
#define kTrackerParamTrackWeightHint "The weight determines the amount this marker contributes to the final solution"

#define kTrackerParamMotionModel "motionModel"
#define kTrackerParamMotionModelLabel "Motion model"
#define kTrackerParamMotionModelHint "The motion model to use for tracking."

#define kTrackerParamMotionModelTranslation "Search for markers that are only translated between frames."
#define kTrackerParamMotionModelTransRot "Search for markers that are translated and rotated between frames."
#define kTrackerParamMotionModelTransScale "Search for markers that are translated and scaled between frames."
#define kTrackerParamMotionModelTransRotScale "Search for markers that are translated, rotated and scaled between frames."
#define kTrackerParamMotionModelAffine "Search for markers that are affine transformed (t,r,k and skew) between frames."
#define kTrackerParamMotionModelPerspective "Search for markers that are perspectively deformed (homography) between frames."

#define kTrackerParamCorrelation "correlation"
#define kTrackerParamCorrelationLabel "Correlation"
#define kTrackerParamCorrelationHint "The correlation score obtained after tracking each frame"

using namespace Natron;


namespace  {
    
enum libmv_MarkerChannel {
    LIBMV_MARKER_CHANNEL_R = (1 << 0),
    LIBMV_MARKER_CHANNEL_G = (1 << 1),
    LIBMV_MARKER_CHANNEL_B = (1 << 2),
};

} // anon namespace

struct TrackMarkerPrivate
{
    boost::weak_ptr<TrackerContext> context;
    
    // Defines the rectangle of the search window, this is in coordinates relative to the marker center point
    boost::shared_ptr<Double_Knob> searchWindowBtmLeft,searchWindowTopRight;
    
    // The pattern Quad defined by 4 corners relative to the center
    boost::shared_ptr<Double_Knob> patternTopLeft,patternTopRight,patternBtmRight,patternBtmLeft;
    boost::shared_ptr<Double_Knob> center,offset,weight,correlation;
    boost::shared_ptr<Choice_Knob> motionModel;
    
    std::list<boost::shared_ptr<KnobI> > knobs;
    
    mutable QMutex trackMutex;
    std::set<int> userKeyframes;
    std::string trackScriptName,trackLabel;
    bool enabled;
    
    TrackMarkerPrivate(const boost::shared_ptr<TrackerContext>& context)
    : context(context)
    , searchWindowBtmLeft()
    , searchWindowTopRight()
    , patternTopLeft()
    , patternTopRight()
    , patternBtmRight()
    , patternBtmLeft()
    , center()
    , offset()
    , weight()
    , correlation()
    , motionModel()
    , knobs()
    , trackMutex()
    , userKeyframes()
    , trackScriptName()
    , trackLabel()
    , enabled(true)
    {
        searchWindowBtmLeft.reset(new Double_Knob(0, kTrackerParamSearchWndBtmLeftLabel, 2, false));
        searchWindowBtmLeft->setName(kTrackerParamSearchWndBtmLeft);
        searchWindowBtmLeft->populate();
        searchWindowBtmLeft->setDefaultValue(-25, 0);
        searchWindowBtmLeft->setDefaultValue(-25, 1);
        knobs.push_back(searchWindowBtmLeft);
        
        searchWindowTopRight.reset(new Double_Knob(0, kTrackerParamSearchWndTopRightLabel, 2, false));
        searchWindowTopRight->setName(kTrackerParamSearchWndTopRight);
        searchWindowTopRight->populate();
        searchWindowTopRight->setDefaultValue(25, 0);
        searchWindowTopRight->setDefaultValue(25, 1);
        knobs.push_back(searchWindowTopRight);
        
        
        patternTopLeft.reset(new Double_Knob(0, kTrackerParamPatternTopLeftLabel, 2, false));
        patternTopLeft->setName(kTrackerParamPatternTopLeft);
        patternTopLeft->populate();
        patternTopLeft->setDefaultValue(-15, 0);
        patternTopLeft->setDefaultValue(15, 1);
        knobs.push_back(patternTopLeft);
        
        patternTopRight.reset(new Double_Knob(0, kTrackerParamPatternTopRightLabel, 2, false));
        patternTopRight->setName(kTrackerParamPatternTopRight);
        patternTopRight->populate();
        patternTopRight->setDefaultValue(15, 0);
        patternTopRight->setDefaultValue(15, 1);
        knobs.push_back(patternTopRight);
        
        patternBtmRight.reset(new Double_Knob(0, kTrackerParamPatternBtmRightLabel, 2, false));
        patternBtmRight->setName(kTrackerParamPatternBtmRight);
        patternBtmRight->populate();
        patternBtmRight->setDefaultValue(15, 0);
        patternBtmRight->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmRight);
        
        patternBtmLeft.reset(new Double_Knob(0, kTrackerParamPatternBtmLeftLabel, 2, false));
        patternBtmLeft->setName(kTrackerParamPatternBtmLeft);
        patternBtmLeft->populate();
        patternBtmLeft->setDefaultValue(-15, 0);
        patternBtmLeft->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmLeft);
        
        center.reset(new Double_Knob(0, kTrackerParamCenterLabel, 2, false));
        center->setName(kTrackerParamCenter);
        center->populate();
        knobs.push_back(center);
        
        offset.reset(new Double_Knob(0, kTrackerParamOffsetLabel, 2, false));
        offset->setName(kTrackerParamOffset);
        offset->populate();
        knobs.push_back(offset);
        
        weight.reset(new Double_Knob(0, kTrackerParamTrackWeightLabel, 1, false));
        weight->setName(kTrackerParamTrackWeight);
        weight->populate();
        weight->setDefaultValue(1.);
        weight->setAnimationEnabled(false);
        weight->setMinimum(0.);
        weight->setMaximum(1.);
        knobs.push_back(weight);
        
        motionModel.reset(new Choice_Knob(0, kTrackerParamMotionModelLabel, 1, false));
        motionModel->setName(kTrackerParamMotionModel);
        motionModel->populate();
        {
            std::vector<std::string> choices,helps;
            choices.push_back("Trans.");
            helps.push_back(kTrackerParamMotionModelTranslation);
            choices.push_back("Trans.+Rot.");
            helps.push_back(kTrackerParamMotionModelTransRot);
            choices.push_back("Trans.+Scale");
            helps.push_back(kTrackerParamMotionModelTransScale);
            choices.push_back("Trans.+Rot.+Scale");
            helps.push_back(kTrackerParamMotionModelTransRotScale);
            choices.push_back("Affine");
            helps.push_back(kTrackerParamMotionModelAffine);
            choices.push_back("Perspective");
            helps.push_back(kTrackerParamMotionModelPerspective);
            motionModel->populateChoices(choices,helps);
        }
        motionModel->setDefaultValue(4);
        knobs.push_back(motionModel);

        correlation.reset(new Double_Knob(0, kTrackerParamCorrelationLabel, 1, false));
        correlation->setName(kTrackerParamCorrelation);
        correlation->populate();
        knobs.push_back(correlation);
    }
};

TrackMarker::TrackMarker(const boost::shared_ptr<TrackerContext>& context)
: _imp(new TrackMarkerPrivate(context))
{
    
}

TrackMarker::~TrackMarker()
{
    
}

boost::shared_ptr<TrackerContext>
TrackMarker::getContext() const
{
    return _imp->context.lock();
}

bool
TrackMarker::setScriptName(const std::string& name)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    
    if (name.empty()) {
        return false;
    }
    
    
    std::string cpy = Natron::makeNameScriptFriendly(name);
    
    if (cpy.empty()) {
        return false;
    }
    
    boost::shared_ptr<TrackMarker> existingItem = getContext()->getMarkerByName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }
    
    {
        QMutexLocker l(&_imp->trackMutex);
        _imp->trackScriptName = cpy;
    }
    return true;
}

std::string
TrackMarker::getScriptName() const
{
    QMutexLocker l(&_imp->trackMutex);
    return _imp->trackScriptName;
}

void
TrackMarker::setLabel(const std::string& label)
{
    QMutexLocker l(&_imp->trackMutex);
    _imp->trackLabel = label;
}

std::string
TrackMarker::getLabel() const
{
    QMutexLocker l(&_imp->trackMutex);
    return _imp->trackLabel;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getSearchWindowBottomLeftKnob() const
{
    return _imp->searchWindowBtmLeft;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getSearchWindowTopRightKnob() const
{
    return _imp->searchWindowTopRight;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getPatternTopLeftKnob() const
{
    return _imp->patternTopLeft;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getPatternTopRightKnob() const
{
    return _imp->patternTopRight;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getPatternBtmRightKnob() const
{
    return _imp->patternBtmRight;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getPatternBtmLeftKnob() const
{
    return _imp->patternBtmLeft;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getWeightKnob() const
{
    return _imp->weight;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getCenterKnob() const
{
    return _imp->center;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getOffsetKnob() const
{
    return _imp->offset;
}

boost::shared_ptr<Double_Knob>
TrackMarker::getCorrelationKnob() const
{
    return _imp->correlation;
}

boost::shared_ptr<Choice_Knob>
TrackMarker::getMotionModelKnob() const
{
    return _imp->motionModel;
}

const std::list<boost::shared_ptr<KnobI> >&
TrackMarker::getKnobs() const
{
    return _imp->knobs;
}

bool
TrackMarker::isUserKeyframe(int time) const
{
    QMutexLocker k(&_imp->trackMutex);
    return _imp->userKeyframes.find(time) != _imp->userKeyframes.end();
}

bool
TrackMarker::isEnabled() const
{
    QMutexLocker k(&_imp->trackMutex);
    return _imp->enabled;
}

void
TrackMarker::setEnabled(bool enabled)
{
    QMutexLocker k(&_imp->trackMutex);
    _imp->enabled = enabled;
}

int
TrackMarker::getReferenceFrame(int time, bool forward) const
{
    QMutexLocker k(&_imp->trackMutex);
    std::set<int>::iterator upper = _imp->userKeyframes.upper_bound(time);
    if (upper == _imp->userKeyframes.end()) {
        //all keys are lower than time, pick the last one
        if (!_imp->userKeyframes.empty()) {
            return *_imp->userKeyframes.rbegin();
        }
        
        // no keyframe - use the previous/next as reference
        return forward ? time - 1 : time + 1;
    } else {
        if (upper == _imp->userKeyframes.begin()) {
            ///all keys are greater than time
            return *upper;
        }
        
        int upperKeyFrame = *upper;
        ///If we find "time" as a keyframe, then use it
        --upper;
        
        int lowerKeyFrame = *upper;
        
        if (lowerKeyFrame == time) {
            return time;
        }
        
        /// return the nearest from time
        return (time - lowerKeyFrame) < (upperKeyFrame - time) ? lowerKeyFrame : upperKeyFrame;
    }
}


struct TrackMarkerAndOptions
{
    boost::shared_ptr<TrackMarker> natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
};

class FrameAccessorImpl;

class TrackArgsLibMV
{
    int _start, _end;
    bool _isForward;
    boost::shared_ptr<TimeLine> _timeline;
    bool _isUpdateViewerEnabled;
    boost::shared_ptr<mv::AutoTrack> _libmvAutotrack;
    boost::shared_ptr<FrameAccessorImpl> _fa;
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > _tracks;
    mutable QMutex _autoTrackMutex;
    
public:
    
    TrackArgsLibMV()
    : _start(0)
    , _end(0)
    , _isForward(false)
    , _timeline()
    , _isUpdateViewerEnabled(false)
    , _libmvAutotrack()
    , _fa()
    , _tracks()
    {
        
    }
    
    TrackArgsLibMV(int start,
                   int end,
                   bool isForward,
                   const boost::shared_ptr<TimeLine>& timeline,
                   bool isUpdateViewerEnabled,
                   const boost::shared_ptr<mv::AutoTrack>& autoTrack,
                   const boost::shared_ptr<FrameAccessorImpl>& fa,
                   const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks)
    : _start(start)
    , _end(end)
    , _isForward(isForward)
    , _timeline(timeline)
    , _isUpdateViewerEnabled(isUpdateViewerEnabled)
    , _libmvAutotrack(autoTrack)
    , _fa(fa)
    , _tracks(tracks)
    {
        
    }
    
    TrackArgsLibMV(const TrackArgsLibMV& other)
    {
        *this = other;
    }
    
    void operator=(const TrackArgsLibMV& other)
    {
        _start = other._start;
        _end = other._end;
        _isForward = other._isForward;
        _timeline = other._timeline;
        _isUpdateViewerEnabled = other._isUpdateViewerEnabled;
        _libmvAutotrack = other._libmvAutotrack;
        _fa = other._fa;
        _tracks = other._tracks;
    }
    
    QMutex* getAutoTrackMutex() const
    {
        return &_autoTrackMutex;
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
        return _isForward;
    }
    
    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }
    
    bool isUpdateViewerEnabled() const
    {
        return _isUpdateViewerEnabled;
    }
    
    int getNumTracks() const
    {
        return (int)_tracks.size();
    }
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& getTracks() const
    {
        return _tracks;
    }
    
    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const
    {
        return _libmvAutotrack;
    }
};


static bool trackStepLibMV(int trackIndex, const TrackArgsLibMV& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < args.getNumTracks());
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks = args.getTracks();
    const boost::shared_ptr<TrackMarkerAndOptions>& track = tracks[trackIndex];
    
    boost::shared_ptr<mv::AutoTrack> autoTrack = args.getLibMVAutoTrack();
    
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    
    assert(track->mvMarker.frame == time);
    
#ifdef DEBUG
    {
        // Make sure the Marker belongs to the AutoTrack
        QMutexLocker k(autoTrackMutex);
        mv::Marker tmp;
        bool ok = autoTrack->GetMarker(0, time, trackIndex, &tmp);
        assert(ok);
    }
#endif
    
    boost::shared_ptr<Double_Knob> searchBtmLeftKnob = track->natronMarker->getSearchWindowBottomLeftKnob();
    boost::shared_ptr<Double_Knob> searchTopRightKnob = track->natronMarker->getSearchWindowTopRightKnob();
    boost::shared_ptr<Double_Knob> offsetKnob = track->natronMarker->getOffsetKnob();
    
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    Natron::Point searchBtmLeft,searchTopRight;
    searchBtmLeft.x = searchBtmLeftKnob->getValueAtTime(time, 0);
    searchBtmLeft.y = searchBtmLeftKnob->getValueAtTime(time, 1);
    
    searchTopRight.x = searchTopRightKnob->getValueAtTime(time, 0);
    searchTopRight.y = searchTopRightKnob->getValueAtTime(time, 1);
    
    
    
    track->mvMarker.search_region.min(0) = searchBtmLeft.x + track->mvMarker.center(0) + offset.x;
    track->mvMarker.search_region.min(1) = searchTopRight.y + track->mvMarker.center(1) + offset.y;
    track->mvMarker.search_region.max(0) = searchTopRight.x + track->mvMarker.center(0) + offset.x;
    track->mvMarker.search_region.max(1) = searchBtmLeft.y + track->mvMarker.center(1) + offset.y;
    
    track->mvMarker.reference_frame = track->natronMarker->getReferenceFrame(time, args.getForward());
    
    if (track->mvMarker.reference_frame == track->mvMarker.frame) {
        // This is a user keyframe, we do not track it
        assert(track->natronMarker->isUserKeyframe(track->mvMarker.frame));
    } else {
        libmv::TrackRegionResult result;
        if (!autoTrack->TrackMarker(&track->mvMarker, &result, &track->mvOptions) || !result.is_usable()) {
            return false;
        }
        
        boost::shared_ptr<Double_Knob> correlationKnob = track->natronMarker->getCorrelationKnob();
        correlationKnob->setValueAtTime(time, result.correlation, 0);
        
        Natron::Point center;
        center.x = (double)track->mvMarker.center(0);
        center.y = (double)track->mvMarker.center(1);
        boost::shared_ptr<Double_Knob> centerKnob = track->natronMarker->getCenterKnob();
        centerKnob->setValuesAtTime(time,
                                    center.x,
                                    center.y,
                                    Natron::eValueChangedReasonNatronInternalEdited);
        
        Natron::Point topLeftCorner,topRightCorner,btmRightCorner,btmLeftCorner;
        topLeftCorner.x = track->mvMarker.patch.coordinates(0,0) - center.x;
        topLeftCorner.y = track->mvMarker.patch.coordinates(0,1) - center.y;
        
        topRightCorner.x = track->mvMarker.patch.coordinates(1,0) - center.x;
        topRightCorner.y = track->mvMarker.patch.coordinates(1,1) - center.y;
        
        btmRightCorner.x = track->mvMarker.patch.coordinates(2,0) - center.x;
        btmRightCorner.y = track->mvMarker.patch.coordinates(2,1) - center.y;
        
        btmLeftCorner.x = track->mvMarker.patch.coordinates(3,0) - center.x;
        btmLeftCorner.y = track->mvMarker.patch.coordinates(3,1) - center.y;
        
        boost::shared_ptr<Double_Knob> pntTopLeftKnob = track->natronMarker->getPatternTopLeftKnob();
        boost::shared_ptr<Double_Knob> pntTopRightKnob = track->natronMarker->getPatternTopRightKnob();
        boost::shared_ptr<Double_Knob> pntBtmLeftKnob = track->natronMarker->getPatternBtmLeftKnob();
        boost::shared_ptr<Double_Knob> pntBtmRightKnob = track->natronMarker->getPatternBtmRightKnob();
        
        pntTopLeftKnob->setValuesAtTime(time, topLeftCorner.x, topLeftCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
        pntTopRightKnob->setValuesAtTime(time, topRightCorner.x, topRightCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
        pntBtmLeftKnob->setValuesAtTime(time, btmLeftCorner.x, btmLeftCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
        pntBtmRightKnob->setValuesAtTime(time, btmRightCorner.x, btmRightCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
        
    } // if (track->mvMarker.reference_frame == track->mvMarker.frame) {
    {
        QMutexLocker k(autoTrackMutex);
        autoTrack->AddMarker(track->mvMarker);
    }
    
    return true;
}

struct TrackerContextPrivate
{
    
    boost::weak_ptr<Natron::Node> node;
    
    std::list<boost::weak_ptr<KnobI> > knobs,perTrackKnobs;
    boost::weak_ptr<Bool_Knob> enableTrackRed,enableTrackGreen,enableTrackBlue;
    boost::weak_ptr<Double_Knob> minCorrelation,maxIterations;
    boost::weak_ptr<Bool_Knob> bruteForcePreTrack,useNormalizedIntensities;
    boost::weak_ptr<Double_Knob> preBlurSigma;
    
    boost::weak_ptr<Double_Knob> searchWindowBtmLeft,searchWindowTopRight;
    boost::weak_ptr<Double_Knob> patternTopLeft,patternTopRight,patternBtmRight,patternBtmLeft;
    boost::weak_ptr<Double_Knob> center,offset,weight,correlation;
    boost::weak_ptr<Choice_Knob> motionModel;
    
    
    mutable QMutex trackerContextMutex;
    std::list<boost::shared_ptr<TrackMarker> > markers;
    std::list<boost::shared_ptr<TrackMarker> > selectedMarkers,markersToSlave,markersToUnslave;
    int beginSelectionCounter;
    
    TrackScheduler<TrackArgsLibMV> scheduler;
    
    TrackerContextPrivate(const boost::shared_ptr<Natron::Node> &node)
    : node(node)
    , knobs()
    , perTrackKnobs()
    , enableTrackRed()
    , enableTrackGreen()
    , enableTrackBlue()
    , minCorrelation()
    , maxIterations()
    , bruteForcePreTrack()
    , useNormalizedIntensities()
    , preBlurSigma()
    , searchWindowBtmLeft()
    , searchWindowTopRight()
    , patternTopLeft()
    , patternTopRight()
    , patternBtmRight()
    , patternBtmLeft()
    , center()
    , offset()
    , weight()
    , correlation()
    , motionModel()
    , trackerContextMutex()
    , markers()
    , selectedMarkers()
    , markersToSlave()
    , markersToUnslave()
    , beginSelectionCounter(0)
    , scheduler(trackStepLibMV)
    {
        Natron::EffectInstance* effect = node->getLiveInstance();
        
        boost::shared_ptr<Page_Knob> settingsPage = Natron::createKnob<Page_Knob>(effect, "Controls", 1 , false);
        
        boost::shared_ptr<Bool_Knob> enableTrackRedKnob = Natron::createKnob<Bool_Knob>(effect, kTrackerParamTrackRedLabel, 1, false);
        enableTrackRedKnob->setName(kTrackerParamTrackRed);
        enableTrackRedKnob->setHintToolTip(kTrackerParamTrackRedHint);
        enableTrackRedKnob->setDefaultValue(true);
        enableTrackRedKnob->setAnimationEnabled(false);
        enableTrackRedKnob->setAddNewLine(false);
        settingsPage->addKnob(enableTrackRedKnob);
        enableTrackRed = enableTrackRedKnob;
        knobs.push_back(enableTrackRedKnob);
        
        boost::shared_ptr<Bool_Knob> enableTrackGreenKnob = Natron::createKnob<Bool_Knob>(effect, kTrackerParamTrackGreenLabel, 1, false);
        enableTrackGreenKnob->setName(kTrackerParamTrackGreen);
        enableTrackGreenKnob->setHintToolTip(kTrackerParamTrackGreenHint);
        enableTrackGreenKnob->setDefaultValue(true);
        enableTrackGreenKnob->setAnimationEnabled(false);
        enableTrackGreenKnob->setAddNewLine(false);
        settingsPage->addKnob(enableTrackGreenKnob);
        enableTrackGreen = enableTrackGreenKnob;
        knobs.push_back(enableTrackGreenKnob);
        
        boost::shared_ptr<Bool_Knob> enableTrackBlueKnob = Natron::createKnob<Bool_Knob>(effect, kTrackerParamTrackBlueLabel, 1, false);
        enableTrackBlueKnob->setName(kTrackerParamTrackBlue);
        enableTrackBlueKnob->setHintToolTip(kTrackerParamTrackBlueHint);
        enableTrackBlueKnob->setDefaultValue(true);
        enableTrackBlueKnob->setAnimationEnabled(false);
        settingsPage->addKnob(enableTrackBlueKnob);
        enableTrackBlue = enableTrackBlueKnob;
        knobs.push_back(enableTrackBlueKnob);
        
        boost::shared_ptr<Double_Knob> minCorelKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamMinimumCorrelationLabel, 1, false);
        minCorelKnob->setName(kTrackerParamMinimumCorrelation);
        minCorelKnob->setHintToolTip(kTrackerParamMinimumCorrelationHint);
        minCorelKnob->setAnimationEnabled(false);
        minCorelKnob->setMinimum(0.);
        minCorelKnob->setMaximum(1.);
        minCorelKnob->setDefaultValue(0.75);
        settingsPage->addKnob(minCorelKnob);
        minCorrelation = minCorelKnob;
        knobs.push_back(minCorelKnob);
        
        boost::shared_ptr<Double_Knob> maxItKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamMaximumIterationLabel, 1, false);
        maxItKnob->setName(kTrackerParamMaximumIteration);
        maxItKnob->setHintToolTip(kTrackerParamMaximumIterationHint);
        maxItKnob->setAnimationEnabled(false);
        maxItKnob->setMinimum(0);
        maxItKnob->setMaximum(150);
        maxItKnob->setDefaultValue(50);
        settingsPage->addKnob(maxItKnob);
        maxIterations = maxItKnob;
        knobs.push_back(maxItKnob);
        
        boost::shared_ptr<Bool_Knob> usePretTrackBF = Natron::createKnob<Bool_Knob>(effect, kTrackerParamBruteForcePreTrackLabel, 1, false);
        usePretTrackBF->setName(kTrackerParamBruteForcePreTrack);
        usePretTrackBF->setHintToolTip(kTrackerParamBruteForcePreTrackHint);
        usePretTrackBF->setDefaultValue(true);
        usePretTrackBF->setAnimationEnabled(false);
        usePretTrackBF->setAddNewLine(false);
        settingsPage->addKnob(usePretTrackBF);
        bruteForcePreTrack = usePretTrackBF;
        knobs.push_back(usePretTrackBF);
        
        boost::shared_ptr<Bool_Knob> useNormalizedInt = Natron::createKnob<Bool_Knob>(effect, kTrackerParamNormalizeIntensitiesLabel, 1, false);
        useNormalizedInt->setName(kTrackerParamNormalizeIntensities);
        useNormalizedInt->setHintToolTip(kTrackerParamNormalizeIntensitiesHint);
        useNormalizedInt->setDefaultValue(false);
        useNormalizedInt->setAnimationEnabled(false);
        settingsPage->addKnob(useNormalizedInt);
        useNormalizedIntensities = useNormalizedInt;
        knobs.push_back(useNormalizedInt);

        boost::shared_ptr<Double_Knob> preBlurSigmaKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamPreBlurSigmaLabel, 1, false);
        preBlurSigmaKnob->setName(kTrackerParamPreBlurSigma);
        preBlurSigmaKnob->setHintToolTip(kTrackerParamPreBlurSigmaHint);
        preBlurSigmaKnob->setAnimationEnabled(false);
        preBlurSigmaKnob->setMinimum(0);
        preBlurSigmaKnob->setMaximum(10.);
        preBlurSigmaKnob->setDefaultValue(0.9);
        settingsPage->addKnob(preBlurSigmaKnob);
        preBlurSigma = preBlurSigmaKnob;
        knobs.push_back(preBlurSigmaKnob);
        
        
        boost::shared_ptr<Double_Knob> sWndBtmLeft = Natron::createKnob<Double_Knob>(effect, kTrackerParamSearchWndBtmLeftLabel, 2, false);
        sWndBtmLeft->setName(kTrackerParamSearchWndBtmLeft);
        sWndBtmLeft->setHintToolTip(kTrackerParamSearchWndBtmLeftHint);
        sWndBtmLeft->setDefaultValue(-25,0);
        sWndBtmLeft->setDefaultValue(-25,1);
        sWndBtmLeft->setMaximum(0, 0);
        sWndBtmLeft->setMaximum(0, 1);
        settingsPage->addKnob(sWndBtmLeft);
        searchWindowBtmLeft = sWndBtmLeft;
        knobs.push_back(sWndBtmLeft);
        perTrackKnobs.push_back(sWndBtmLeft);
        
        boost::shared_ptr<Double_Knob> sWndTopRight = Natron::createKnob<Double_Knob>(effect, kTrackerParamSearchWndTopRightLabel, 2, false);
        sWndTopRight->setName(kTrackerParamSearchWndTopRight);
        sWndTopRight->setHintToolTip(kTrackerParamSearchWndTopRightHint);
        sWndTopRight->setDefaultValue(25,0);
        sWndTopRight->setDefaultValue(25,1);
        sWndTopRight->setMinimum(0, 0);
        sWndTopRight->setMinimum(0, 1);
        settingsPage->addKnob(sWndTopRight);
        searchWindowTopRight = sWndTopRight;
        knobs.push_back(sWndTopRight);
        perTrackKnobs.push_back(sWndTopRight);
        
        boost::shared_ptr<Double_Knob> ptnTopLeft = Natron::createKnob<Double_Knob>(effect, kTrackerParamPatternTopLeftLabel, 2, false);
        ptnTopLeft->setName(kTrackerParamPatternTopLeft);
        ptnTopLeft->setHintToolTip(kTrackerParamPatternTopLeftHint);
        ptnTopLeft->setDefaultValue(-15,0);
        ptnTopLeft->setDefaultValue(15,1);
        ptnTopLeft->setMaximum(0, 0);
        ptnTopLeft->setMinimum(0, 1);
        settingsPage->addKnob(ptnTopLeft);
        patternTopLeft = ptnTopLeft;
        knobs.push_back(ptnTopLeft);
        perTrackKnobs.push_back(ptnTopLeft);
        
        boost::shared_ptr<Double_Knob> ptnTopRight = Natron::createKnob<Double_Knob>(effect, kTrackerParamPatternTopRightLabel, 2, false);
        ptnTopRight->setName(kTrackerParamPatternTopRight);
        ptnTopRight->setHintToolTip(kTrackerParamPatternTopRightHint);
        ptnTopRight->setDefaultValue(15,0);
        ptnTopRight->setDefaultValue(15,1);
        ptnTopRight->setMinimum(0, 0);
        ptnTopRight->setMinimum(0, 1);
        settingsPage->addKnob(ptnTopRight);
        patternTopRight = ptnTopRight;
        knobs.push_back(ptnTopRight);
        perTrackKnobs.push_back(ptnTopRight);
        
        boost::shared_ptr<Double_Knob> ptnBtmRight = Natron::createKnob<Double_Knob>(effect, kTrackerParamPatternBtmRightLabel, 2, false);
        ptnBtmRight->setName(kTrackerParamPatternBtmRight);
        ptnBtmRight->setHintToolTip(kTrackerParamPatternBtmRightHint);
        ptnBtmRight->setDefaultValue(15,0);
        ptnBtmRight->setDefaultValue(-15,1);
        ptnBtmRight->setMinimum(0, 0);
        ptnBtmRight->setMaximum(0, 1);
        settingsPage->addKnob(ptnBtmRight);
        patternBtmRight = ptnBtmRight;
        knobs.push_back(ptnBtmRight);
        perTrackKnobs.push_back(ptnBtmRight);

        
        boost::shared_ptr<Double_Knob> ptnBtmLeft = Natron::createKnob<Double_Knob>(effect, kTrackerParamPatternBtmLeftLabel, 2, false);
        ptnBtmLeft->setName(kTrackerParamPatternBtmLeft);
        ptnBtmLeft->setHintToolTip(kTrackerParamPatternBtmLeftHint);
        ptnBtmLeft->setDefaultValue(-15,0);
        ptnBtmLeft->setDefaultValue(-15,1);
        ptnBtmLeft->setMaximum(0, 0);
        ptnBtmLeft->setMaximum(0, 1);
        settingsPage->addKnob(ptnBtmLeft);
        patternBtmLeft = ptnBtmLeft;
        knobs.push_back(ptnBtmLeft);
        perTrackKnobs.push_back(ptnBtmLeft);
        
        boost::shared_ptr<Double_Knob> centerKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamCenterLabel, 2, false);
        centerKnob->setName(kTrackerParamCenter);
        centerKnob->setHintToolTip(kTrackerParamCenterHint);
        settingsPage->addKnob(centerKnob);
        center = centerKnob;
        knobs.push_back(centerKnob);
        perTrackKnobs.push_back(centerKnob);

        boost::shared_ptr<Double_Knob> offsetKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamOffsetLabel, 2, false);
        offsetKnob->setName(kTrackerParamOffset);
        offsetKnob->setHintToolTip(kTrackerParamOffsetHint);
        settingsPage->addKnob(offsetKnob);
        offset = offsetKnob;
        knobs.push_back(offsetKnob);
        perTrackKnobs.push_back(offsetKnob);
        
        boost::shared_ptr<Double_Knob> weightKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamTrackWeightLabel, 1, false);
        weightKnob->setName(kTrackerParamTrackWeight);
        weightKnob->setHintToolTip(kTrackerParamTrackWeightHint);
        weightKnob->setAnimationEnabled(false);
        weightKnob->setMinimum(0.);
        weightKnob->setMaximum(1.);
        weightKnob->setDefaultValue(1.);
        settingsPage->addKnob(weightKnob);
        weight = weightKnob;
        knobs.push_back(weightKnob);
        perTrackKnobs.push_back(weightKnob);
        
        boost::shared_ptr<Double_Knob> correlationKnob = Natron::createKnob<Double_Knob>(effect, kTrackerParamCorrelationLabel, 1, false);
        correlationKnob->setName(kTrackerParamCorrelation);
        correlationKnob->setHintToolTip(kTrackerParamCorrelationHint);
        correlationKnob->setAnimationEnabled(false);
        correlationKnob->setMinimum(0.);
        correlationKnob->setMaximum(1.);
        correlationKnob->setDefaultValue(1.);
        settingsPage->addKnob(correlationKnob);
        correlation = correlationKnob;
        knobs.push_back(correlationKnob);
        perTrackKnobs.push_back(correlationKnob);
        
        boost::shared_ptr<Choice_Knob> motionModelKnob = Natron::createKnob<Choice_Knob>(effect, kTrackerParamMotionModelLabel, 1, false);
        motionModelKnob->setName(kTrackerParamMotionModel);
        motionModelKnob->setHintToolTip(kTrackerParamMotionModelHint);
        {
            std::vector<std::string> choices,helps;
            choices.push_back("Trans.");
            helps.push_back(kTrackerParamMotionModelTranslation);
            choices.push_back("Trans.+Rot.");
            helps.push_back(kTrackerParamMotionModelTransRot);
            choices.push_back("Trans.+Scale");
            helps.push_back(kTrackerParamMotionModelTransScale);
            choices.push_back("Trans.+Rot.+Scale");
            helps.push_back(kTrackerParamMotionModelTransRotScale);
            choices.push_back("Affine");
            helps.push_back(kTrackerParamMotionModelAffine);
            choices.push_back("Perspective");
            helps.push_back(kTrackerParamMotionModelPerspective);
            motionModelKnob->populateChoices(choices,helps);
        }
        motionModelKnob->setAnimationEnabled(false);
        motionModelKnob->setMinimum(0.);
        motionModelKnob->setMaximum(1.);
        motionModelKnob->setDefaultValue(4);
        settingsPage->addKnob(motionModelKnob);
        motionModel = motionModelKnob;
        knobs.push_back(motionModelKnob);
        perTrackKnobs.push_back(motionModelKnob);
        
        
    }
    
    /// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
    void natronTrackerToLibMVTracker(bool trackChannels[3],
                                     const TrackMarker& marker,
                                     int time,
                                     bool forward,
                                     mv::Marker* mvMarker) const;
    
    /// Make all calls to getValue() that are global to the tracker context in here
    void beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const;
    
    /// Make all calls to getValue() that are local to the track in here
    void endLibMVOptionsForTrack(const TrackMarker& marker,
                                  mv::TrackRegionOptions* options) const;
    
    void addToSelectionList(const boost::shared_ptr<TrackMarker>& marker)
    {
        if (std::find(selectedMarkers.begin(), selectedMarkers.end(), marker) != selectedMarkers.end()) {
            return;
        }
        selectedMarkers.push_back(marker);
        markersToSlave.push_back(marker);
    }
    
    void removeFromSelectionList(const boost::shared_ptr<TrackMarker>& marker)
    {
        std::list<boost::shared_ptr<TrackMarker> >::iterator found = std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);
        if (found == selectedMarkers.end()) {
            return;
        }
        selectedMarkers.erase(found);
        markersToUnslave.push_back(marker);
    }
    
    void incrementSelectionCounter()
    {
        ++beginSelectionCounter;
    }
    
    void decrementSelectionCounter()
    {
        if (beginSelectionCounter > 0) {
            --beginSelectionCounter;
        }
    }
    
    
};

TrackerContext::TrackerContext(const boost::shared_ptr<Natron::Node> &node)
: _imp(new TrackerContextPrivate(node))
{
    
}

TrackerContext::~TrackerContext()
{
    
}

boost::shared_ptr<TrackMarker>
TrackerContext::getMarkerByName(const std::string & name) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if ((*it)->getScriptName() == name) {
            return *it;
        }
    }
    return boost::shared_ptr<TrackMarker>();
}

std::string
TrackerContext::generateUniqueTrackName(const std::string& baseName)
{
    int no = 1;
    
    bool foundItem;
    std::string name;
    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();
        if (getMarkerByName(name)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    return name;
}

boost::shared_ptr<TrackMarker>
TrackerContext::createMarker()
{
    boost::shared_ptr<TrackMarker> track(new TrackMarker(shared_from_this()));
    std::string name = generateUniqueTrackName(kTrackBaseName);
    track->setScriptName(name);
    track->setLabel(name);
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        _imp->markers.push_back(track);
    }
    return track;
    
}

void
TrackerContext::removeMarker(const TrackMarker* marker)
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if (it->get() == marker) {
            _imp->markers.erase(it);
            return;
        }
    }
    
}

boost::shared_ptr<Natron::Node>
TrackerContext::getNode() const
{
    return _imp->node.lock();
}

int
TrackerContext::getTimeLineFirstFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)first;
}

int
TrackerContext::getTimeLineLastFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)last;
}

void
TrackerContextPrivate::natronTrackerToLibMVTracker(bool trackChannels[3],
                                                   const TrackMarker& marker,
                                                   int time,
                                                   bool forward,
                                                   mv::Marker* mvMarker) const
{
    
    
    boost::shared_ptr<Double_Knob> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<Double_Knob> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<Double_Knob> patternTopLeftKnob = marker.getPatternTopLeftKnob();
    boost::shared_ptr<Double_Knob> patternTopRightKnob = marker.getPatternTopRightKnob();
    boost::shared_ptr<Double_Knob> patternBtmRightKnob = marker.getPatternBtmRightKnob();
    boost::shared_ptr<Double_Knob> patternBtmLeftKnob = marker.getPatternBtmLeftKnob();
    boost::shared_ptr<Double_Knob> weightKnob = marker.getWeightKnob();
    boost::shared_ptr<Double_Knob> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<Double_Knob> offsetKnob = marker.getOffsetKnob();
    
    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;
    mvMarker->weight = (float)weightKnob->getValue();
    mvMarker->frame = time;
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    mvMarker->center(0) = centerKnob->getValueAtTime(time, 0);
    mvMarker->center(1) = centerKnob->getValueAtTime(time, 1);
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    
    mvMarker->disabled_channels =
    (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
    (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
    (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);
    
    Natron::Point searchWndBtmLeft,searchWndTopRight;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(time, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(time, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(time, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(time, 1);
    
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + mvMarker->center(0);
    mvMarker->search_region.min(1) = searchWndTopRight.y + mvMarker->center(1);
    mvMarker->search_region.max(0) = searchWndTopRight.x + mvMarker->center(0);
    mvMarker->search_region.max(1) = searchWndBtmLeft.y + mvMarker->center(1);
    
    Natron::Point tl,tr,br,bl;
    tl.x = patternTopLeftKnob->getValueAtTime(time, 0);
    tl.y = patternTopLeftKnob->getValueAtTime(time, 1);

    tr.x = patternTopRightKnob->getValueAtTime(time, 0);
    tr.y = patternTopRightKnob->getValueAtTime(time, 1);
    
    br.x = patternBtmRightKnob->getValueAtTime(time, 0);
    br.y = patternBtmRightKnob->getValueAtTime(time, 1);
    
    bl.x = patternBtmLeftKnob->getValueAtTime(time, 0);
    bl.y = patternBtmLeftKnob->getValueAtTime(time, 1);
    
    mvMarker->patch.coordinates(0,0) = tl.x + mvMarker->center(0);
    mvMarker->patch.coordinates(0,1) = tl.y + mvMarker->center(1);
    
    mvMarker->patch.coordinates(1,0) = tr.x + mvMarker->center(0);
    mvMarker->patch.coordinates(1,1) = tr.y + mvMarker->center(1);
    
    mvMarker->patch.coordinates(2,0) = br.x + mvMarker->center(0);
    mvMarker->patch.coordinates(2,1) = br.y + mvMarker->center(1);
    
    mvMarker->patch.coordinates(3,0) = bl.x + mvMarker->center(0);
    mvMarker->patch.coordinates(3,1) = bl.y + mvMarker->center(1);
}

void
TrackerContextPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    options->minimum_correlation = minCorrelation.lock()->getValue();
    options->max_iterations = maxIterations.lock()->getValue();
    options->use_brute_initialization = bruteForcePreTrack.lock()->getValue();
    options->use_normalized_intensities = useNormalizedIntensities.lock()->getValue();
    options->sigma = preBlurSigma.lock()->getValue();

}

void
TrackerContextPrivate::endLibMVOptionsForTrack(const TrackMarker& marker,
                              mv::TrackRegionOptions* options) const
{
    int mode_i = marker.getMotionModelKnob()->getValue();
    switch (mode_i) {
        case 0:
            options->mode = mv::TrackRegionOptions::TRANSLATION;
            break;
        case 1:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION;
            break;
        case 2:
            options->mode = mv::TrackRegionOptions::TRANSLATION_SCALE;
            break;
        case 3:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
            break;
        case 4:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
        case 5:
            options->mode = mv::TrackRegionOptions::HOMOGRAPHY;
            break;
        default:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
    }
}

struct FrameAccessorCacheKey
{
    int frame;
    int mipMapLevel;
    mv::FrameAccessor::InputMode mode;
};

struct CacheKey_compare_less
{
    bool operator() (const FrameAccessorCacheKey & lhs,
                     const FrameAccessorCacheKey & rhs) const
    {
        if (lhs.frame < rhs.frame) {
            return true;
        } else if (lhs.frame > rhs.frame) {
            return false;
        } else {
            if (lhs.mipMapLevel < rhs.mipMapLevel) {
                return true;
            } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
                return false;
            } else {
                if ((int)lhs.mode < (int)rhs.mode) {
                    return true;
                } else if ((int)lhs.mode > (int)rhs.mode) {
                    return false;
                } else {
                    return false;
                }
            }
                
        }
    }
};


class MvFloatImage : public libmv::Array3D<float>
{
    
public:
    
    MvFloatImage()
    : libmv::Array3D<float>()
    {
        
    }
    
    MvFloatImage(int height, int width)
    : libmv::Array3D<float>(height, width)
    {
        
    }
    
    MvFloatImage(float* data, int height, int width)
    : libmv::Array3D<float>(data, height, width)
    {
        
    }
    
    virtual ~MvFloatImage()
    {
        
    }
};

struct FrameAccessorCacheEntry
{
    boost::shared_ptr<MvFloatImage> image;
    
    // If null, this is the full image
    RectI bounds;
    
    unsigned int referenceCount;
};

class FrameAccessorImpl : public mv::FrameAccessor
{
    const TrackerContext* _context;
    boost::shared_ptr<Natron::Node> _trackerInput;
    typedef std::multimap<FrameAccessorCacheKey, FrameAccessorCacheEntry, CacheKey_compare_less > FrameAccessorCache;
    
    mutable QMutex _cacheMutex;
    FrameAccessorCache _cache;
    bool _enabledChannels[3];
    
public:
    
    FrameAccessorImpl(const TrackerContext* context,
                      bool enabledChannels[3])
    : _context(context)
    , _trackerInput()
    , _cacheMutex()
    , _cache()
    {
        _trackerInput = context->getNode()->getInput(0);
        assert(_trackerInput);
        for (int i = 0; i < 3; ++i) {
            _enabledChannels[i] = enabledChannels[i];
        }
    }
    
    virtual ~FrameAccessorImpl()
    {
        
    }
    
    // Get a possibly-filtered version of a frame of a video. Downscale will
    // cause the input image to get downscaled by 2^downscale for pyramid access.
    // Region is always in original-image coordinates, and describes the
    // requested area. The transform describes an (optional) transform to apply
    // to the image before it is returned.
    //
    // When done with an image, you must call ReleaseImage with the returned key.
    virtual mv::FrameAccessor::Key GetImage(int clip,
                                            int frame,
                                            mv::FrameAccessor::InputMode input_mode,
                                            int downscale,               // Downscale by 2^downscale.
                                            const mv::Region* region,        // Get full image if NULL.
                                            const mv::FrameAccessor::Transform* transform,  // May be NULL.
                                            mv::FloatImage* destination) OVERRIDE FINAL;
    
    // Releases an image from the frame accessor. Non-caching implementations may
    // free the image immediately; others may hold onto the image.
    virtual void ReleaseImage(Key) OVERRIDE FINAL;
    
    virtual bool GetClipDimensions(int clip, int* width, int* height) OVERRIDE FINAL;
    virtual int NumClips() OVERRIDE FINAL;
    virtual int NumFrames(int clip) OVERRIDE FINAL;
    
private:
    
    
};


template <bool doR, bool doG, bool doB>
void natronImageToLibMvFloatImageForChannels(const Natron::Image* source,
                                             const RectI& roi,
                                             MvFloatImage& mvImg)
{
    //mvImg is expected to have its bounds equal to roi
    
    Natron::Image::ReadAccess racc(source);
    
    unsigned int compsCount = source->getComponentsCount();
    assert(compsCount == 3);
    unsigned int srcRowElements = source->getRowElements();
    
    
    const float* src_pixels = (const float*)racc.pixelAt(roi.x1, roi.y2 - 1);
    assert(src_pixels);
    float* dst_pixels = mvImg.Data();
    assert(dst_pixels);
    //LibMV images have their origin in the top left hand corner
    
    // It's important to rescale the resultappropriately so that e.g. if only
    // blue is selected, it's not zeroed out.
    float scale = (doR ? 0.2126f : 0.0f) +
    (doG ? 0.7152f : 0.0f) +
    (doB ? 0.0722f : 0.0f);
    
    int h = roi.height();
    int w = roi.width();
    for (int y = 0; y < h ; ++y,
         src_pixels += (srcRowElements - compsCount * w)) {
        
        for (int x = 0; x < w; ++x,
             src_pixels += compsCount,
             ++dst_pixels) {
            
            /// Apply luminance conversion while we copy the image
            /// This code is taken from DisableChannelsTransform::run in libmv/autotrack/autotrack.cc
            *dst_pixels = (0.2126f * (doR ? src_pixels[0] : 0.0f) +
                           0.7152f * (doG ? src_pixels[1] : 0.0f) +
                           0.0722f * (doB ? src_pixels[2] : 0.0f)) / scale;
        }
    }
}


void natronImageToLibMvFloatImage(bool enabledChannels[3],
                                  const Natron::Image* source,
                                  const RectI& roi,
                                  MvFloatImage& mvImg)
{
    
    if (enabledChannels[0]) {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,false,false>(source,roi,mvImg);
            }
        }
        
    } else {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,false,false>(source,roi,mvImg);
            }
        }
    }
}

mv::FrameAccessor::Key
FrameAccessorImpl::GetImage(int /*clip*/,
                            int frame,
                            mv::FrameAccessor::InputMode input_mode,
                            int downscale,               // Downscale by 2^downscale.
                            const mv::Region* region,        // Get full image if NULL.
                            const mv::FrameAccessor::Transform* /*transform*/,  // May be NULL.
                            mv::FloatImage* destination)
{
    
    // Since libmv only uses MONO images for now we have only optimized for this case, remove and handle properly
    // other case(s) when they get integrated into libmv.
    assert(input_mode == mv::FrameAccessor::MONO);
    
    
    
    FrameAccessorCacheKey key;
    key.frame = frame;
    key.mipMapLevel = downscale;
    key.mode = input_mode;
    
    {
        QMutexLocker k(&_cacheMutex);
        std::pair<FrameAccessorCache::iterator,FrameAccessorCache::iterator> range = _cache.equal_range(key);
        for (FrameAccessorCache::iterator it = range.first; it != range.second; ++it) {
            if (it->second.bounds.isNull() ||
                (region && region->min(0) >= it->second.bounds.x1 && region->min(1) <= it->second.bounds.y2 &&
                 region->max(0) <= it->second.bounds.x2 && region->max(1) >= it->second.bounds.y1)) {
                    // LibMV is kinda dumb on this we must necessarily copy the data...
                    destination->CopyFrom<float>(*it->second.image);
                    ++it->second.referenceCount;
                    return (mv::FrameAccessor::Key)it->second.image.get();
                }
        }
    }
    
    // Not in accessor cache, call renderRoI
    RenderScale scale;
    scale.y = scale.x = Image::getScaleFromMipMapLevel((unsigned int)downscale);
    
    RectI roi;
    RectD precomputedRoD;
    if (region) {
        roi.x1 = region->min(0);
        roi.x2 = region->max(0);
        roi.y1 = region->max(1);
        roi.y2 = region->min(1);
    } else {
        bool isProjectFormat;
        Natron::StatusEnum stat = _trackerInput->getLiveInstance()->getRegionOfDefinition_public(_trackerInput->getHashValue(), frame, scale, 0, &precomputedRoD, &isProjectFormat);
        if (stat == eStatusFailed) {
            return (mv::FrameAccessor::Key)0;
        }
        double par = _trackerInput->getLiveInstance()->getPreferredAspectRatio();
        precomputedRoD.toPixelEnclosing((unsigned int)downscale, par, &roi);
    }
    
    std::list<ImageComponents> components;
    components.push_back(ImageComponents::getRGBComponents());
    
    NodePtr node = _context->getNode();
    
    ParallelRenderArgsSetter frameRenderArgs(node->getApp()->getProject().get(),
                                             frame,
                                             0, //<  view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             false, //can abort
                                             0, //render Age
                                             0, // viewer requester
                                             0, //texture index
                                             node->getApp()->getTimeLine().get(),
                                             NodePtr(),
                                             true);

    
    EffectInstance::RenderRoIArgs args(frame,
                                       scale,
                                       downscale,
                                       0,
                                       false,
                                       roi,
                                       precomputedRoD,
                                       components,
                                       Natron::eImageBitDepthFloat,
                                       _context->getNode()->getLiveInstance());
    ImageList planes;
    EffectInstance::RenderRoIRetCode stat = _trackerInput->getLiveInstance()->renderRoI(args, &planes);
    if (stat != EffectInstance::eRenderRoIRetCodeOk || planes.empty()) {
        return (mv::FrameAccessor::Key)0;
    }
    
    assert(!planes.empty());
    const ImagePtr& sourceImage = planes.front();
    
    RectI intersectedRoI;
    roi.intersect(sourceImage->getBounds(), &intersectedRoI);
    
    FrameAccessorCacheEntry entry;
    entry.image.reset(new MvFloatImage(roi.height(), roi.width()));
    entry.bounds = roi;
    entry.referenceCount = 1;
    natronImageToLibMvFloatImage(_enabledChannels,
                                 sourceImage.get(),
                                 roi,
                                 *entry.image);
    // we ignore the transform parameter and do it in natronImageToLibMvFloatImage instead
    
    // LibMV is kinda dumb on this we must necessarily copy the data...
    destination->CopyFrom<float>(*entry.image);
    
    //insert into the cache
    {
        QMutexLocker k(&_cacheMutex);
        _cache.insert(std::make_pair(key,entry));
    }
    return (mv::FrameAccessor::Key)entry.image.get();
}

void
FrameAccessorImpl::ReleaseImage(Key key)
{
    MvFloatImage* imgKey = (MvFloatImage*)key;
    QMutexLocker k(&_cacheMutex);
    for (FrameAccessorCache::iterator it = _cache.begin(); it != _cache.end(); ++it) {
        if (it->second.image.get() == imgKey) {
            --it->second.referenceCount;
            if (!it->second.referenceCount) {
                _cache.erase(it);
                return;
            }
        }
    }
}

// Not used in LibMV
bool
FrameAccessorImpl::GetClipDimensions(int /*clip*/, int* /*width*/, int* /*height*/)
{
    return false;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumClips()
{
    return 1;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumFrames(int /*clip*/)
{
    return 0;
}

void
TrackerContext::trackSelectedMarkers(int start, int end, bool forward, bool updateViewer)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = _imp->selectedMarkers.begin(); it != _imp->selectedMarkers.end(); ++it) {
            if ((*it)->isEnabled()) {
                markers.push_back(*it);
            }
        }
    }
    
    if (markers.empty()) {
        return;
    }
    
    /// The channels we are going to use for tracking
    bool enabledChannels[3];
    enabledChannels[0] = _imp->enableTrackRed.lock()->getValue();
    enabledChannels[1] = _imp->enableTrackGreen.lock()->getValue();
    enabledChannels[2] = _imp->enableTrackBlue.lock()->getValue();
    
    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    boost::shared_ptr<FrameAccessorImpl> accessor(new FrameAccessorImpl(this, enabledChannels));
    boost::shared_ptr<mv::AutoTrack> trackContext(new mv::AutoTrack(accessor.get()));
    
    
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > trackAndOptions;
    
    mv::TrackRegionOptions mvOptions;
    _imp->beginLibMVOptionsForTrack(&mvOptions);
    
    int trackIndex = 0;
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!= markers.end(); ++it, ++trackIndex) {
        boost::shared_ptr<TrackMarkerAndOptions> t(new TrackMarkerAndOptions);
        t->natronMarker = *it;
        _imp->natronTrackerToLibMVTracker(enabledChannels, *t->natronMarker, start, forward, &t->mvMarker);
        t->mvMarker.track = trackIndex;
        t->mvOptions = mvOptions;
        _imp->endLibMVOptionsForTrack(*t->natronMarker, &t->mvOptions);
        
        trackContext->AddMarker(t->mvMarker);
        
        trackAndOptions.push_back(t);
    }
    
    
    
    TrackArgsLibMV args(start, end, forward, getNode()->getApp()->getTimeLine(), updateViewer, trackContext, accessor, trackAndOptions);
    _imp->scheduler.track(args);
}

void
TrackerContext::beginEditSelection()
{
    QMutexLocker k(&_imp->trackerContextMutex);
    _imp->incrementSelectionCounter();
}

void
TrackerContext::endEditSelection(TrackSelectionReason reason)
{
    QMutexLocker k(&_imp->trackerContextMutex);
    _imp->decrementSelectionCounter();
    if (_imp->beginSelectionCounter == 0) {
        endSelection(reason);
    }
}

void
TrackerContext::addTrackToSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > marks;
    marks.push_back(mark);
    addTracksToSelection(marks, reason);
}

void
TrackerContext::addTracksToSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason)
{
    QMutexLocker k(&_imp->trackerContextMutex);
    
    bool hasCalledBegin = false;
    if (!_imp->beginSelectionCounter) {
        _imp->incrementSelectionCounter();
        hasCalledBegin = true;
    }
    
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
        _imp->addToSelectionList(*it);
    }
    
    if (hasCalledBegin) {
        _imp->decrementSelectionCounter();
        endSelection(reason);
    }
}

void
TrackerContext::removeTrackFromSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > marks;
    marks.push_back(mark);
    removeTracksFromSelection(marks, reason);
}

void
TrackerContext::removeTracksFromSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason)
{
    QMutexLocker k(&_imp->trackerContextMutex);
    
    bool hasCalledBegin = false;
    if (!_imp->beginSelectionCounter) {
        _imp->incrementSelectionCounter();
        hasCalledBegin = true;
    }
    
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
        _imp->removeFromSelectionList(*it);
    }
    
    if (hasCalledBegin) {
        _imp->decrementSelectionCounter();
        endSelection(reason);
    }

}

void
TrackerContext::clearSelection(TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getSelectedMarkers(&markers);
    removeTracksFromSelection(markers, reason);
}

void
TrackerContext::getSelectedMarkers(std::list<boost::shared_ptr<TrackMarker> >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    *markers = _imp->selectedMarkers;
}

void
TrackerContext::endSelection(TrackSelectionReason reason)
{
    assert(!_imp->trackerContextMutex.tryLock());
    
    // Slave newly selected knobs
    bool selectionIsDirty = _imp->selectedMarkers.size() > 1;
    bool selectionEmpty = _imp->selectedMarkers.empty();
    
    {
        std::list<boost::shared_ptr<TrackMarker> >::const_iterator next = _imp->markersToSlave.begin();
        if (!_imp->markersToSlave.empty()) {
            ++next;
        }
        
        
        for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markersToSlave.begin() ; it!=_imp->markersToSlave.end(); ++it) {
            const std::list<boost::shared_ptr<KnobI> >& trackKnobs = (*it)->getKnobs();
            for (std::list<boost::shared_ptr<KnobI> >::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
                
                // Find the knob in the TrackerContext knobs
                boost::shared_ptr<KnobI> found;
                for (std::list<boost::weak_ptr<KnobI> >::iterator it3 = _imp->perTrackKnobs.begin(); it3 != _imp->perTrackKnobs.end(); ++it3) {
                    boost::shared_ptr<KnobI> k = it3->lock();
                    if (k->getName() == (*it2)->getName()) {
                        found = k;
                        break;
                    }
                }
                
                if (!found) {
                    continue;
                }
                
                //Clone current state only for the last marker
                if (next == _imp->markersToSlave.end()) {
                    found->cloneAndUpdateGui(it2->get());
                }
                
                //Slave internal knobs of the bezier
                assert((*it2)->getDimension() == found->getDimension());
                for (int i = 0; i < (*it2)->getDimension(); ++i) {
                    (*it2)->slaveTo(i, found, i);
                }
                
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(SequenceTime,int,int,bool)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(SequenceTime,int,int)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(SequenceTime,int)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(SequenceTime,int)),
                                 this, SLOT(onSelectedKnobCurveChanged()));
                
                
            }
            if (next != _imp->markersToSlave.end()) {
                ++next;
            }
        } // for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markersToSlave.begin() ; it!=_imp->markersToSlave.end(); ++it)
        _imp->markersToSlave.clear();
    }
    
    {
        std::list<boost::shared_ptr<TrackMarker> >::const_iterator next = _imp->markersToUnslave.begin();
        if (!_imp->markersToUnslave.empty()) {
            ++next;
        }
        for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markersToUnslave.begin() ; it!=_imp->markersToUnslave.end(); ++it) {
            const std::list<boost::shared_ptr<KnobI> >& trackKnobs = (*it)->getKnobs();
            for (std::list<boost::shared_ptr<KnobI> >::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
                
                // Find the knob in the TrackerContext knobs
                boost::shared_ptr<KnobI> found;
                for (std::list<boost::weak_ptr<KnobI> >::iterator it3 = _imp->perTrackKnobs.begin(); it3 != _imp->perTrackKnobs.end(); ++it3) {
                    boost::shared_ptr<KnobI> k = it3->lock();
                    if (k->getName() == (*it2)->getName()) {
                        found = k;
                        break;
                    }
                }
                
                if (!found) {
                    continue;
                }
                
                //Clone current state only for the last marker
                if (next == _imp->markersToSlave.end()) {
                    found->cloneAndUpdateGui(it2->get());
                }
                
                //Slave internal knobs of the bezier
                assert((*it2)->getDimension() == found->getDimension());
                for (int i = 0; i < (*it2)->getDimension(); ++i) {
                    (*it2)->unSlave(i, !selectionIsDirty);
                }
                
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(SequenceTime,int,int,bool)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(SequenceTime,int,int)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(SequenceTime,int)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(SequenceTime,int)),
                                    this, SLOT(onSelectedKnobCurveChanged()));
                
                
            }
            if (next != _imp->markersToUnslave.end()) {
                ++next;
            }
        } // for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markersToUnslave() ; it!=_imp->markersToUnslave(); ++it)
        _imp->markersToUnslave.clear();
    }
    
    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->perTrackKnobs.begin(); it != _imp->perTrackKnobs.end(); ++it) {
        boost::shared_ptr<KnobI> k = it->lock();
        if (!k) {
            continue;
        }
        k->setAllDimensionsEnabled(!selectionEmpty);
        k->setDirty(selectionIsDirty);
    }
    
    Q_EMIT selectionChanged((int)reason);
}

void
TrackerContext::onSelectedKnobCurveChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        boost::shared_ptr<KnobI> knob = handler->getKnob();
        for (std::list<boost::weak_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (k->getName() == knob->getName()) {
                k->clone(knob.get());
                break;
            }
        }
    }
}

//////////////////////// TrackScheduler

struct TrackSchedulerPrivate
{
    
    mutable QMutex mustQuitMutex;
    bool mustQuit;
    QWaitCondition mustQuitCond;
    
    mutable QMutex abortRequestedMutex;
    int abortRequested;
    QWaitCondition abortRequestedCond;
    
    QMutex startRequesstMutex;
    int startRequests;
    QWaitCondition startRequestsCond;
    
    mutable QMutex isWorkingMutex;
    bool isWorking;
    
    
    TrackSchedulerPrivate()
    : mustQuitMutex()
    , mustQuit(false)
    , mustQuitCond()
    , abortRequestedMutex()
    , abortRequested(0)
    , abortRequestedCond()
    , startRequesstMutex()
    , startRequests(0)
    , startRequestsCond()
    , isWorkingMutex()
    , isWorking(false)
    {
        
    }
    
    bool checkForExit()
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeAll();
            return true;
        }
        return false;
    }
    
};

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::TrackScheduler(TrackStepFunctor functor)
: TrackSchedulerBase()
, _imp(new TrackSchedulerPrivate())
, argsMutex()
, curArgs()
, requestedArgs()
, _functor(functor)
{
    setObjectName("TrackScheduler");
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::~TrackScheduler()
{
    
}

template <class TrackArgsType>
bool
TrackScheduler<TrackArgsType>::isWorking() const
{
    QMutexLocker k(&_imp->isWorkingMutex);
    return _imp->isWorking;
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::run()
{
    for (;;) {
        
        ///Check for exit of the thread
        if (_imp->checkForExit()) {
            return;
        }
        
        ///Flag that we're working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = true;
        }
        
        ///Copy the requested args to the args used for processing
        {
            QMutexLocker k(&argsMutex);
            curArgs = requestedArgs;
        }
        
        boost::shared_ptr<TimeLine> timeline = curArgs.getTimeLine();
        
        int end = curArgs.getEnd();
        int start = curArgs.getStart();
        int cur = start;
        bool isForward = curArgs.getForward();
        int framesCount = isForward ? (end - start) : (start - end);
        bool isUpdateViewerOnTrackingEnabled = curArgs.isUpdateViewerEnabled();
        
        int numTracks = curArgs.getNumTracks();
        std::vector<int> trackIndexes;
        for (std::size_t i = 0; i < (std::size_t)numTracks; ++i) {
            trackIndexes.push_back(i);
        }
        
        bool reportProgress = numTracks > 1 || framesCount > 1;
        if (reportProgress) {
            emit_trackingStarted();
        }
        
        while (cur != end) {
            
            
            
            ///Launch parallel thread for each track using the global thread pool
            QFuture<bool> future = QtConcurrent::mapped(trackIndexes,
                              boost::bind(_functor,
                                          _1,
                                          curArgs,
                                          cur));
            future.waitForFinished();
            
            for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                if (!(*it)) {
                    break;
                }
            }
            
            double progress;
            if (isForward) {
                ++cur;
                progress = (double)(cur - start) / framesCount;
            } else {
                --cur;
                progress = (double)(start - cur) / framesCount;
            }
            
            ///Ok all tracks are finished now for this frame, refresh viewer if needed
            if (isUpdateViewerOnTrackingEnabled) {
                timeline->seekFrame(cur, true, 0, Natron::eTimelineChangeReasonUserSeek);
            }
            
            if (reportProgress) {
                ///Notify we progressed of 1 frame
                emit_progressUpdate(progress);
            }
            
            ///Check for abortion
            {
                QMutexLocker k(&_imp->abortRequestedMutex);
                if (_imp->abortRequested > 0) {
                    _imp->abortRequested = 0;
                    _imp->abortRequestedCond.wakeAll();
                    break;
                }
            }
            
        }
        
        if (reportProgress) {
            emit_trackingFinished();
        }
        
        ///Flag that we're no longer working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = false;
        }
        
        ///Make sure we really reset the abort flag
        {
            QMutexLocker k(&_imp->abortRequestedMutex);
            if (_imp->abortRequested > 0) {
                _imp->abortRequested = 0;
                
            }
        }
        
        ///Sleep or restart if we've requests in the queue
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            while (_imp->startRequests <= 0) {
                _imp->startRequestsCond.wait(&_imp->startRequesstMutex);
            }
            _imp->startRequests = 0;
        }
        
    }
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::track(const TrackArgsType& args)
{
    if ((args.getForward() && args.getStart() >= args.getEnd()) || (!args.getForward() && args.getStart() <= args.getEnd())) {
        Q_EMIT trackingFinished();
        return;
    }
    {
        QMutexLocker k(&argsMutex);
        requestedArgs = args;
    }
    if (isRunning()) {
        QMutexLocker k(&_imp->startRequesstMutex);
        ++_imp->startRequests;
        _imp->startRequestsCond.wakeAll();
    } else {
        start();
    }
}


template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::abortTracking()
{
    if (!isRunning() || !isWorking()) {
        return;
    }
    
    
    {
        QMutexLocker k(&_imp->abortRequestedMutex);
        ++_imp->abortRequested;
        _imp->abortRequestedCond.wakeAll();
    }
    
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    abortTracking();
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeAll();
        }
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
        
    }
    
    
    wait();
    
}
