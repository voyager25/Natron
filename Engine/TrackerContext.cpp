//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "TrackerContext.h"

#include <set>

#include "Engine/Node.h"
#include "Engine/KnobTypes.h"

using namespace Natron;

struct TrackMarkerPrivate
{
    boost::weak_ptr<TrackerContext> context;
    
    boost::shared_ptr<Double_Knob> searchWindowBtmLeft,searchWindowTopRight;
    boost::shared_ptr<Double_Knob> patternTopLeft,patternTopRight,patternBtmRight,patternBtmLeft;
    boost::shared_ptr<Double_Knob> center,offset;
    
    mutable QMutex trackMutex;
    std::set<int> userKeyframes;
    
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
    , trackMutex()
    , userKeyframes()
    {
        
    }
};

TrackMarker::TrackMarker(const boost::shared_ptr<TrackerContext>& context)
: _imp(new TrackMarkerPrivate(context))
{
    
}


struct TrackerContextPrivate
{
    
    boost::weak_ptr<Natron::Node> node;
    
    TrackerContextPrivate(const boost::shared_ptr<Natron::Node> &node)
    : node(node)
    {
        
    }
};

TrackerContext::TrackerContext(const boost::shared_ptr<Natron::Node> &node)
: _imp(new TrackerContextPrivate(node))
{
}

TrackerContext::~TrackerContext()
{
    
}