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
    
private:
    
    boost::scoped_ptr<TrackerPanelPrivate> _imp;
};

#endif // TRACKERPANEL_H
