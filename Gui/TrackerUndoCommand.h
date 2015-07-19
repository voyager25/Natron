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


#ifndef TRACKERUNDOCOMMAND_H
#define TRACKERUNDOCOMMAND_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <list>
#include <QUndoCommand>

#include "Global/Macros.h"

class TrackMarker;
class TrackerPanel;
class TrackerContext;

class AddTrackCommand
: public QUndoCommand
{
    
public:
    
    AddTrackCommand(const boost::shared_ptr<TrackMarker> &marker, const boost::shared_ptr<TrackerContext>& context);
    
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
    
private:
    
    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
   
    std::list<boost::shared_ptr<TrackMarker> > _markers;
    boost::weak_ptr<TrackerContext> _context;
    
};


class RemoveTracksCommand
: public QUndoCommand
{

public:
    
    RemoveTracksCommand(const std::list<boost::shared_ptr<TrackMarker> > &markers, const boost::shared_ptr<TrackerContext>& context);
    
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    

private:
    
    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    struct TrackToRemove
    {
        boost::shared_ptr<TrackMarker> track;
        boost::weak_ptr<TrackMarker> prevTrack;
    };
    
    std::list<TrackToRemove> _markers;
    boost::weak_ptr<TrackerContext> _context;
    
};


#endif // TRACKERUNDOCOMMAND_H
