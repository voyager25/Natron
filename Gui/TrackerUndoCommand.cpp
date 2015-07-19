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


#include "TrackerUndoCommand.h"

#include "Engine/TrackerContext.h"
#include "Gui/TrackerPanel.h"


AddTrackCommand::AddTrackCommand(const boost::shared_ptr<TrackMarker> &marker, const boost::shared_ptr<TrackerContext>& context)
: QUndoCommand()
, _markers()
, _context(context)
{
    _markers.push_back(marker);
    setText(QObject::tr("Add Track(s)"));
}

void
AddTrackCommand::undo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();
    if (!context) {
        return;
    }
    
    context->beginEditSelection();
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->removeMarker(*it);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}

void
AddTrackCommand::redo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();
    if (!context) {
        return;
    }
    context->beginEditSelection();
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->addTrackToSelection(*it, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}

RemoveTracksCommand::RemoveTracksCommand(const std::list<boost::shared_ptr<TrackMarker> > &markers, const boost::shared_ptr<TrackerContext>& context)
: QUndoCommand()
, _markers()
, _context(context)
{
    assert(!_markers.empty());
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin(); it!=markers.end(); ++it) {
        TrackToRemove t;
        t.track = *it;
        t.prevTrack = context->getPrevMarker(t.track, false);
        _markers.push_back(t);
    }
    setText(QObject::tr("Remove Track(s)"));
}

void
RemoveTracksCommand::undo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();
    if (!context) {
        return;
    }
    context->beginEditSelection();
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        int prevIndex = -1 ;
        boost::shared_ptr<TrackMarker> prevMarker = it->prevTrack.lock();
        if (prevMarker) {
            prevIndex = context->getMarkerIndex(prevMarker);
        }
        if (prevIndex != -1) {
            context->insertMarker(it->track, prevIndex);
        } else {
            context->appendMarker(it->track);
        }
        context->addTrackToSelection(it->track, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}

void
RemoveTracksCommand::redo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();
    if (!context) {
        return;
    }
    
    boost::shared_ptr<TrackMarker> nextMarker = context->getNextMarker(_markers.back().track, true);
    
    context->beginEditSelection();
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->removeMarker(it->track);
    }
    if (nextMarker) {
        context->addTrackToSelection(nextMarker, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}