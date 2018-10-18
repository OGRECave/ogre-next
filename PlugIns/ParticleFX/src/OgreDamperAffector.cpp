/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreDamperAffector.h"
#include "OgreParticleSystem.h"
#include "OgreParticle.h"
#include "OgreStringConverter.h"


namespace Ogre {

    // Instantiate statics
    DamperAffector::CmdConstantDamping  DamperAffector::msConstantDampingCmd;
    DamperAffector::CmdLinearDamping    DamperAffector::msLinearDampingCmd;
    DamperAffector::CmdQuadraticDamping DamperAffector::msQuadraticDampingCmd;


    //-----------------------------------------------------------------------
    DamperAffector::DamperAffector(ParticleSystem* psys)
        :ParticleAffector(psys)
    {
        mType = "Damper";

        mConstantDamping = 0.1;
        mLinearDamping = 0.01;
        mQuadraticDamping = 0.002;

        // Set up parameters
        if (createParamDictionary("DamperAffector"))
        {
            addBaseParameters();
            // Add extra parameters
            ParamDictionary* dict = getParamDictionary();
            dict->addParameter(ParameterDef("constant", 
                "Constant deceleration.",
                PT_REAL),&msConstantDampingCmd);
            dict->addParameter(ParameterDef("linear", 
                "Deceleration linearly proportional to particle speed.",
                PT_REAL),&msLinearDampingCmd);
            dict->addParameter(ParameterDef("quadratic", 
                "Deceleration quadratically proportional to particle speed.",
                PT_REAL),&msQuadraticDampingCmd);
        }

    }
    //-----------------------------------------------------------------------
    void DamperAffector::_affectParticles(ParticleSystem* pSystem, Real timeElapsed)
    {
        ParticleIterator pi = pSystem->_getIterator();
        Particle *p;

        while (!pi.end())
        {
            p = pi.getNext();
            
            Vector3 velocity = p->mDirection;
            Real speed = velocity.length();
            
            if (speed > 0.0001) {
                Vector3 dir = velocity/speed;
                Real damping = mConstantDamping + mLinearDamping * speed + mQuadraticDamping * speed * speed;
                p->mDirection -= dir * damping * timeElapsed;
                
                // if it changed direction stop it
                if (p->mDirection.dotProduct(velocity) < 0) {
                    p->mDirection = Vector3::ZERO;
                }
            }
        }
        
    }
    //-----------------------------------------------------------------------
    void DamperAffector::setConstantDamping(Real d)
    {
        mConstantDamping = d;
    }
    //-----------------------------------------------------------------------
    void DamperAffector::setLinearDamping(Real d)
    {
        mLinearDamping = d;
    }
    //-----------------------------------------------------------------------
    void DamperAffector::setQuadraticDamping(Real d)
    {
        mQuadraticDamping = d;
    }
    //-----------------------------------------------------------------------
    Real DamperAffector::getConstantDamping(void) const
    {
        return mConstantDamping;
    }
    //-----------------------------------------------------------------------
    Real DamperAffector::getLinearDamping(void) const
    {
        return mLinearDamping;
    }
    //-----------------------------------------------------------------------
    Real DamperAffector::getQuadraticDamping(void) const
    {
        return mQuadraticDamping;
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Command objects
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String DamperAffector::CmdConstantDamping::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const DamperAffector*>(target)->getConstantDamping() );
    }
    void DamperAffector::CmdConstantDamping::doSet(void* target, const String& val)
    {
        static_cast<DamperAffector*>(target)->setConstantDamping(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String DamperAffector::CmdLinearDamping::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const DamperAffector*>(target)->getLinearDamping() );
    }
    void DamperAffector::CmdLinearDamping::doSet(void* target, const String& val)
    {
        static_cast<DamperAffector*>(target)->setLinearDamping(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    String DamperAffector::CmdQuadraticDamping::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const DamperAffector*>(target)->getQuadraticDamping() );
    }
    void DamperAffector::CmdQuadraticDamping::doSet(void* target, const String& val)
    {
        static_cast<DamperAffector*>(target)->setQuadraticDamping(
            StringConverter::parseReal(val));
    }

}
