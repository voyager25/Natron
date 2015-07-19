//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "TrackerNode.h"
#include "Engine/Node.h"

using namespace Natron;

TrackerNode::TrackerNode(boost::shared_ptr<Natron::Node> node)
: Natron::EffectInstance(node)
{
}

TrackerNode::~TrackerNode()
{
    
}

std::string
TrackerNode::getPluginID() const
{
    return PLUGINID_NATRON_TRACKER;
}


std::string
TrackerNode::getPluginLabel() const
{
    return "Tracker";
}

std::string
TrackerNode::getDescription() const
{
    return "";
}

std::string
TrackerNode::getInputLabel (int inputNb) const
{
    switch (inputNb) {
        case 0:
            return "Source";
        case 1:
            return "Mask";
    }
    return "";
}

bool
TrackerNode::isInputMask(int inputNb) const
{
    return inputNb == 1;
}

bool
TrackerNode::isInputOptional(int inputNb) const
{
    return inputNb == 1;
}

void
TrackerNode::addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps)
{
    
    if (inputNb != 1) {
        comps->push_back(ImageComponents::getRGBAComponents());
        comps->push_back(ImageComponents::getRGBComponents());
        comps->push_back(ImageComponents::getXYComponents());
    }
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
TrackerNode::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

void
TrackerNode::getPreferredDepthAndComponents(int inputNb,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const
{
    
    if (inputNb != 1) {
        comp->push_back(ImageComponents::getRGBAComponents());
    } else {
        comp->push_back(ImageComponents::getAlphaComponents());
    }
    *depth = eImageBitDepthFloat;
}


Natron::ImagePremultiplicationEnum
TrackerNode::getOutputPremultiplication() const
{
    
    EffectInstance* input = getInput(0);
    Natron::ImagePremultiplicationEnum srcPremult = eImagePremultiplicationOpaque;
    if (input) {
        srcPremult = input->getOutputPremultiplication();
    }
    bool processA = getNode()->getProcessChannel(3);
    if (srcPremult == eImagePremultiplicationOpaque && processA) {
        return eImagePremultiplicationUnPremultiplied;
    }
    return eImagePremultiplicationPremultiplied;
}

double
TrackerNode::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }
    
}


void
TrackerNode::initializeKnobs()
{
    
}

bool
TrackerNode::isIdentity(double /*time*/,
                        const RenderScale & /*scale*/,
                        const RectI & /*roi*/,
                        int /*view*/,
                        double* /*inputTime*/,
                        int* /*inputNb*/)
{
    // Identity for now, until we can apply a transform
    return true;
}

