//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef NODEGRAPH_H
#define NODEGRAPH_H
#include <cmath>
#include <QGraphicsView>
#include <iostream>
#include <QtCore/QRectF>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QInputDialog>
#include <QLabel>
#include <QUndoCommand>
#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif
#include "Superviser/powiterFn.h"

class QVBoxLayout;
class Node;
class NodeGui;
class QScrollArea;
class Controler;
class Edge;
class SmartInputDialog;
class QGraphicsProxyWidget;
class SettingsPanel;
class QUndoStack;

class NodeGraph: public QGraphicsView , public boost::noncopyable{
    enum EVENT_STATE{DEFAULT,MOVING_AREA,ARROW_DRAGGING,NODE_DRAGGING};
    Q_OBJECT
    
    class NodeGraphNavigator : public QLabel{
        int _w,_h;
    public:
        
        NodeGraphNavigator(QWidget* parent = 0);
        
        void setImage(const QImage& img);
        
        virtual QSize sizeHint() const {return QSize(_w,_h);};
        
        virtual ~NodeGraphNavigator(){}
    };

public:

    NodeGraph(QGraphicsScene* scene=0,QWidget *parent=0);

    virtual ~NodeGraph();
 
    void setPropertyBinPtr(QScrollArea* propertyBin){_propertyBin = propertyBin;}
    
    void createNodeGUI(QVBoxLayout *dockContainer,Node *node);
    
    void removeNode(NodeGui* n);

    virtual void enterEvent(QEvent *event);
    
    virtual void leaveEvent(QEvent *event);

    virtual void keyPressEvent(QKeyEvent *e);
    
    virtual bool event(QEvent* event);
    
    void autoConnect(NodeGui* selected,NodeGui* created);
    
    void setSmartNodeCreationEnabled(bool enabled){smartNodeCreationEnabled=enabled;}

    void checkIfViewerConnectedAndRefresh(NodeGui* n);
    
    void selectNode(NodeGui* n);
    
    QRectF visibleRect();
    
    QRectF visibleRect_v2();
    
    void deselect();
    
    QImage getFullSceneScreenShot();
    
    bool areAllNodesVisible();
    
    void updateNavigator();
protected:

    void mousePressEvent(QMouseEvent *event);
    
    void mouseReleaseEvent(QMouseEvent *event);
    
    void mouseMoveEvent(QMouseEvent *event);
    
    void mouseDoubleClickEvent(QMouseEvent *event);

    void wheelEvent(QWheelEvent *event);

    void scaleView(qreal scaleFactor,QPointF center);

private:
    
    void deleteSelectedNode();
    
    void autoResizeScene();
    
    bool smartNodeCreationEnabled;
    QPointF old_pos;
    QPointF oldp;
    QPointF _lastNodeDragStartPoint;
    QPointF oldZoom;
    EVENT_STATE _evtState;
    NodeGui* _nodeSelected;
    Edge* _arrowSelected;
    std::vector<NodeGui*> _nodes;
    bool _nodeCreationShortcutEnabled;
    bool _fullscreen;
    QGraphicsItem* _root;
    QScrollArea* _propertyBin;
    
    NodeGraphNavigator* _navigator;
    QGraphicsProxyWidget* _navigatorProxy;
    
    QUndoStack* _undoStack;
    
    QAction* _undoAction,*_redoAction;
    
};


class MoveCommand : public QUndoCommand{
public:
    MoveCommand(NodeGui *node, const QPointF &oldPos,
                QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    NodeGui* _node;
    QPointF _oldPos;
    QPointF _newPos;
};


class AddCommand : public QUndoCommand{
public:

    AddCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    
private:
    std::vector<NodeGui*> _children; //edges of the children that are pointing to this node
    std::vector<NodeGui*> _parents;
    NodeGui* _node;
    NodeGraph* _graph;
    bool _undoWasCalled;
};

class RemoveCommand : public QUndoCommand{
public:
    
    RemoveCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    
private:
    std::vector<NodeGui*> _children; //edges of the children that are pointing to this node
    std::vector<NodeGui*> _parents;
    NodeGui* _node;
    NodeGraph* _graph;
};

class ConnectCommand : public QUndoCommand{
public:
    ConnectCommand(NodeGraph* graph,Edge* edge,NodeGui *oldSrc,NodeGui* newSrc,QUndoCommand *parent = 0);
    
    virtual void undo();
    virtual void redo();
private:
    Edge* _edge;
    NodeGui *_oldSrc,*_newSrc;
    NodeGraph* _graph;
};

#endif // NODEGRAPH_H
