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


#ifndef TRACKERPANEL_H
#define TRACKERPANEL_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

class NodeGui;
class TrackerContext;
class TrackMarker;
class TableItem;
class KnobI;

class QUndoCommand;
class QItemSelection;

/**
* @brief This is the new tracker panel, the previous version TrackerPanelV1 (used for TrackerPM) can be found in MultiInstancePanel.h
**/
struct TrackerPanelPrivate;
class TrackerPanel
: public QWidget
{
        Q_OBJECT

public:
    
    TrackerPanel(const boost::shared_ptr<NodeGui>& n,
                 QWidget* parent);
    
    virtual ~TrackerPanel();
    
    void addTableRow(const boost::shared_ptr<TrackMarker> & marker);
    
    void insertTableRow(const boost::shared_ptr<TrackMarker> & marker, int index);
    
    void removeRow(int row);
    
    void removeMarker(const boost::shared_ptr<TrackMarker> & marker);
    
    TableItem* getItemAt(int row, int column) const;
    
    int getMarkerRow(const boost::shared_ptr<TrackMarker> & marker) const;
    
    boost::shared_ptr<TrackMarker> getRowMarker(int row) const;
    
    boost::shared_ptr<KnobI> getKnobAt(int row, int column, int* dimension) const;
    
    boost::shared_ptr<TrackerContext> getContext() const;
    
    boost::shared_ptr<NodeGui> getNode() const;
    
    void getSelectedRows(std::set<int>* rows) const;
    
    void blockSelection();
    void unblockSelection();
    
    void pushUndoCommand(QUndoCommand* command);
    
    void clearAndSelectTracks(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason);
    
public Q_SLOTS:
    
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onSelectAllButtonClicked();
    void onResetButtonClicked();
    void onAverageButtonClicked();
    void onExportButtonClicked();
    
    void onModelSelectionChanged(const QItemSelection & oldSelection,const QItemSelection & newSelection);
    void onItemDataChanged(TableItem* item);
    void onItemEnabledCheckBoxChecked(bool checked);
    void onItemMotionModelChanged(int index);
    void onItemRightClicked(TableItem* item);
    
    void onContextSelectionChanged(int reason);
    
    void onTrackKeyframeSet(const boost::shared_ptr<TrackMarker>& marker, int key);
    void onTrackKeyframeRemoved(const boost::shared_ptr<TrackMarker>& marker, int key);
    void onTrackAllKeyframesRemoved(const boost::shared_ptr<TrackMarker>& marker);
    
    void onKeyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int key);
    void onKeyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker, int key);
    void onAllKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker> marker);
    void onMultipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker> marker, const std::list<int>& keys);
    
private:
    
    void selectInternal(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason);
    boost::shared_ptr<TrackMarker> makeTrackInternal();
    
    boost::scoped_ptr<TrackerPanelPrivate> _imp;
};

#endif // TRACKERPANEL_H
