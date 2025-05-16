// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file DwaBaseMaterial.cc

#include "attributes.cc"
#include "labels.h"
#include "DwaBaseMaterial_ispc_stubs.h"

#include <moonshine/material/dwabase/DwaBase.h>
#include <moonshine/material/dwabase/DwaBaseLayerable.h>
#include <moonray/rendering/shading/MaterialApi.h>

#include <string>

using namespace scene_rdl2::math;
using namespace moonray::shading;
using namespace moonshine::dwabase;

namespace {

DECLARE_DWA_BASE_LABELS()

DwaBaseAttributeKeys
collectAttributeKeys()
{
    DwaBaseAttributeKeys keys;
    ASSIGN_GLITTER_ATTR_KEYS(keys);
    ASSIGN_IRIDESCENCE_ATTR_KEYS(keys);

    keys.mShowFuzz                      = attrShowFuzz;
    keys.mFuzz                          = attrFuzz;
    keys.mFuzzAlbedo                    = attrFuzzAlbedo;
    keys.mFuzzRoughness                 = attrFuzzRoughness;
    keys.mFuzzUseAbsorbingFibers        = attrFuzzUseAbsorbingFibers;
    keys.mFuzzNormal                    = attrFuzzNormal;
    keys.mFuzzNormalDial                = attrFuzzNormalDial;

    keys.mShowSpecular                  = attrShowSpecular;
    keys.mSpecular                      = attrSpecular;
    keys.mSpecularModel                 = attrSpecularModel;
    keys.mRefractiveIndex               = attrRefractiveIndex;
    keys.mMetallic                      = attrMetallic;
    keys.mMetallicColor                 = attrMetallicColor;
    keys.mMetallicEdgeColor             = attrMetallicEdgeColor;
    keys.mRoughness                     = attrRoughness;
    keys.mAnisotropy                    = attrAnisotropy;
    keys.mShadingTangent                = attrShadingTangent;
    keys.mShowTransmission              = attrShowTransmission;
    keys.mTransmission                  = attrTransmission;
    keys.mTransmissionColor             = attrTransmissionColor;
    keys.mUseIndependentTransmissionRefractiveIndex= attrUseIndependentTransmissionRefractiveIndex;
    keys.mIndependentTransmissionRefractiveIndex   = attrIndependentTransmissionRefractiveIndex;
    keys.mUseIndependentTransmissionRoughness      = attrUseIndependentTransmissionRoughness;
    keys.mIndependentTransmissionRoughness         = attrIndependentTransmissionRoughness;
    keys.mUseDispersion                 = attrUseDispersion;
    keys.mDispersionAbbeNumber          = attrDispersionAbbeNumber;
    keys.mShowDiffuse                   = attrShowDiffuse;
    keys.mAlbedo                        = attrAlbedo;
    keys.mDiffuseRoughness              = attrDiffuseRoughness;
    keys.mSubsurface                    = attrSubsurface;
    keys.mScatteringColor               = attrScatteringColor;
    keys.mScatteringRadius              = attrScatteringRadius;
    keys.mSubsurfaceTraceSet            = attrSubsurfaceTraceSet;
    keys.mEnableSubsurfaceInputNormal   = attrEnableSubsurfaceInputNormal;
    keys.mSSSResolveSelfIntersections   = attrSSSResolveSelfIntersections;
    keys.mDiffuseTransmission           = attrDiffuseTransmission;
    keys.mDiffuseTransmissionColor      = attrDiffuseTransmissionColor;
    keys.mDiffuseTransmissionBlendingBehavior = attrDiffuseTransmissionBlendingBehavior;
    keys.mShowOuterSpecular             = attrShowClearcoat;
    keys.mOuterSpecular                 = attrClearcoat;
    keys.mOuterSpecularModel            = attrClearcoatModel;
    keys.mOuterSpecularRefractiveIndex  = attrClearcoatRefractiveIndex;
    keys.mOuterSpecularRoughness        = attrClearcoatRoughness;
    keys.mOuterSpecularThickness        = attrClearcoatThickness;
    keys.mOuterSpecularAttenuationColor = attrClearcoatAttenuationColor;
    keys.mOuterSpecularUseBending       = attrClearcoatUseBending;
    keys.mUseOuterSpecularNormal        = attrUseClearcoatNormal;
    keys.mOuterSpecularNormal           = attrClearcoatNormal;
    keys.mOuterSpecularNormalDial       = attrClearcoatNormalDial;
    keys.mShowEmission                  = attrShowEmission;
    keys.mEmission                      = attrEmission;
    keys.mPresence                      = attrPresence;
    keys.mInputNormal                   = attrInputNormal;
    keys.mInputNormalDial               = attrInputNormalDial;
    keys.mNormalAAStrategy              = attrNormalAAStrategy;
    keys.mNormalAADial                  = attrNormalAADial;
    keys.mThinGeometry                  = attrThinGeometry;
    keys.mCastsCaustics                 = attrCastsCaustics;

    return keys;
}

} // end anonymous namespace

RDL2_DSO_CLASS_BEGIN(DwaBaseMaterial, DwaBase)

public:
    DwaBaseMaterial(const SceneClass& sceneClass, const std::string& name);
    ~DwaBaseMaterial() { }

private:
    void update() override;
    static void shade(const Material* self, moonray::shading::TLState *tls,
                      const State& state, BsdfBuilder& bsdfBuilder);

    static Vec3f evalSubsurfaceNormal(const Material* self,
                                      moonray::shading::TLState *tls,
                                      const moonray::shading::State& state);

RDL2_DSO_CLASS_END(DwaBaseMaterial)

DwaBaseMaterial::DwaBaseMaterial(const SceneClass& sceneClass, const std::string& name) :
    DwaBase(sceneClass,
            name,
            collectAttributeKeys(),
            ispc::DwaBaseMaterial_collectAttributeFuncs(),
            sLabels,
            ispc::Model::Mixed)
{
    mType |= INTERFACE_DWABASELAYERABLE;

    mShadeFunc = DwaBaseMaterial::shade;
    mShadeFuncv = (ShadeFuncv) ispc::DwaBaseMaterial_getShadeFunc();
}

void
DwaBaseMaterial::update()
{
    // must call DwaBase::update()!
    DwaBase::update();

    // set bssrdf normal map
    ispc::DwaBase* dwabase = getISPCBaseMaterialStruct();
    dwabase->mAttrFuncs.mEvalSubsurfaceNormal = getEnableSubsurfaceInputNormal() ?
            (intptr_t)DwaBaseMaterial::evalSubsurfaceNormal : 0;

    if (get(attrDiffuseLightSet)) {
        dwabase->mDiffuseLightSet = get(attrDiffuseLightSet)->asA<scene_rdl2::rdl2::LightSet>();
    }
    if (get(attrSpecularLightSet)) {
        dwabase->mSpecularLightSet = get(attrSpecularLightSet)->asA<scene_rdl2::rdl2::LightSet>();
    }
}

void
DwaBaseMaterial::shade(const Material* self,
                       moonray::shading::TLState *tls,
                       const State& state,
                       BsdfBuilder& bsdfBuilder)
{
    const DwaBaseMaterial* me = static_cast<const DwaBaseMaterial*>(self);
    const ispc::DwaBase* dwabase = me->getISPCBaseMaterialStruct();

    ispc::DwaBaseParameters params;
    me->resolveParameters(tls, state, me->get(attrCastsCaustics), params);
    me->createLobes(me, tls, state, bsdfBuilder, params, dwabase->mUParams, sLabels);
}

Vec3f
DwaBaseMaterial::evalSubsurfaceNormal(const Material* self,
                                      moonray::shading::TLState *tls,
                                      const moonray::shading::State& state)
{
    const DwaBaseMaterial* me = static_cast<const DwaBaseMaterial*>(self);
    return me->resolveSubsurfaceNormal(tls, state);
}

