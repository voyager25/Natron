//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef TRACKERCONTEXT_H
#define TRACKERCONTEXT_H

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

namespace Natron {
    class Node;
}
class TrackerContext;
struct TrackMarkerPrivate;
class TrackMarker
{
public:
    
    TrackMarker(const boost::shared_ptr<TrackerContext>& context);
    
    ~TrackMarker();
    
private:
    
    boost::scoped_ptr<TrackMarkerPrivate> _imp;
    
};

struct TrackerContextPrivate;
class TrackerContext
{
public:
    
    TrackerContext(const boost::shared_ptr<Natron::Node> &node);
    
    ~TrackerContext();
    
private:
    
    boost::scoped_ptr<TrackerContextPrivate> _imp;
};

#endif // TRACKERCONTEXT_H
