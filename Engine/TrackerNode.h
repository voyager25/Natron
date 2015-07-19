//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef TRACKERNODE_H
#define TRACKERNODE_H

#include "Engine/EffectInstance.h"

class TrackerNode : Natron::EffectInstance
{
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new TrackerNode(n);
    }
    
    TrackerNode(boost::shared_ptr<Natron::Node> node);
    
    virtual ~TrackerNode();
    
    virtual bool isBuiltinTrackerNode() const OVERRIDE FINAL WARN_UNUSED_RETURN {
        return true;
    }
    
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 2;
    }
    
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    
    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual std::string getDescription() const WARN_UNUSED_RETURN;
    
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_TRANSFORM);
    }
    
    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isInputMask(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL;
    
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    
    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return Natron::eRenderSafetyFullySafeFrame;
    }
    
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }
    
    virtual void initializeKnobs() OVERRIDE FINAL;
    
    virtual void getPreferredDepthAndComponents(int inputNb,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const OVERRIDE FINAL;
    
    virtual Natron::ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL;
    
    virtual double getPreferredAspectRatio() const OVERRIDE FINAL;
    
    virtual bool isHostMaskingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    virtual bool isHostMixingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return false; }
    
    
    virtual bool isHostChannelSelectorSupported(bool* /*defaultR*/,bool* /*defaultG*/, bool* /*defaultB*/, bool* /*defaultA*/) const OVERRIDE WARN_UNUSED_RETURN {
        return false;
    }
    
private:
    
    virtual bool isIdentity(double time,
                            const RenderScale & scale,
                            const RectI & roi,
                            int view,
                            double* inputTime,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

#endif // TRACKERNODE_H
